/*testRecordMain.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include <epicsExport.h>

#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/event.h>

#include "testRecord.h"

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

class MyPVListener;
typedef std::tr1::shared_ptr<MyPVListener> MyPVListenerPtr;

class MyProcessRequester;
typedef std::tr1::shared_ptr<MyProcessRequester> MyProcessRequesterPtr;

class MyPutRequester;
typedef std::tr1::shared_ptr<MyPutRequester> MyPutRequesterPtr;

class MyPVListener :
   public PVListener,
   public std::tr1::enable_shared_from_this<MyPVListener>
{
public:
    POINTER_DEFINITIONS(PVListener);
    static MyPVListenerPtr create(
        String const & requesterName,
        PVRecordPtr const & pvRecord,
        PVRecordFieldPtr const &pvRecordField)
    {
        MyPVListenerPtr pvPVListener(
            new MyPVListener(requesterName,pvRecord,pvRecordField));
        pvPVListener->init();
        return pvPVListener;
    }
    virtual ~MyPVListener() {}
    virtual void detach(PVRecordPtr const & pvRecord)
    {
        printf("%s MyPVListener::detach\n",requesterName.c_str());
    }
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField)
    {
        String fieldName = pvRecordField->getFullFieldName();
        printf("%s dataPut(requested(%s))\n",
            requesterName.c_str(),
            fieldName.c_str());
    }
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField)
    {
        String requestedName = requested->getFullFieldName();
        String fieldName = pvRecordField->getFullFieldName();
        printf("%s dataPut(requested(%s),pvRecordField(%s))\n",
            requesterName.c_str(),
            requestedName.c_str(),
            fieldName.c_str());
    }
    virtual void beginGroupPut(PVRecordPtr const & pvRecord)
    {
        printf("beginGroupPut\n");
    }
    virtual void endGroupPut(PVRecordPtr const & pvRecord)
    {
        printf("endGroupPut\n");
    }
    virtual void unlisten(PVRecordPtr const & pvRecord)
    {
        printf("unlisten\n");
    }
private:
    MyPVListenerPtr getPtrSelf()
    {
        return shared_from_this();
    }

    MyPVListener(
        String const & requesterName,
        PVRecordPtr const & pvRecord,
        PVRecordFieldPtr const &pvRecordField)
    : isDetached(false),
      requesterName(requesterName),
      pvRecord(pvRecord),
      pvRecordField(pvRecordField)
    {}
    void init() {
        pvRecord->addPVRecordClient(getPtrSelf());
        pvRecord->addListener(getPtrSelf());
        pvRecordField->addListener(getPtrSelf());
    }
    
    bool isDetached;
    String requesterName;
    PVRecordPtr pvRecord;
    PVRecordFieldPtr pvRecordField;
};

class MyProcessRequester :
   public virtual Requester,
   public virtual RecordProcessRequester,
   public std::tr1::enable_shared_from_this<MyProcessRequester>
{
public:
    static MyProcessRequesterPtr create(
        String const &requesterName,
        PVRecordPtr const & pvRecord)
    {
        MyProcessRequesterPtr pvProcessRequester(
            new MyProcessRequester(requesterName,pvRecord));
        pvProcessRequester->init();
        return pvProcessRequester;
    }
    bool process()
    {
        if(isDetached) {
            message("process request but detached",errorMessage);
            return false;
        }
        pvRecord->queueProcessRequest(getPtrSelf());
        event.wait();
        return true;
    }
    virtual ~MyProcessRequester() {}
    virtual void detach(PVRecordPtr const & pvRecord)
    { 
        isDetached = true;
        message("detached",infoMessage);
    }
    virtual String getRequesterName() {return requesterName;}
    virtual void message(
        String const & message,
        MessageType messageType)
    {
        String messageTypeName = getMessageTypeName(messageType);
        printf("%s %s %s\n",
            requesterName.c_str(),
            message.c_str(),
            messageTypeName.c_str());
    }
    virtual void recordDestroyed()
    {
        printf("%s MyProcessRequester::recordDestroyed\n",
            requesterName.c_str());
    }
    virtual void becomeProcessor()
    {
         pvRecord->process(getPtrSelf(),false);
    }
    virtual void recordProcessResult(epics::pvData::Status status)
    {
        String xxx("recordProcessResult ");
        message( xxx + status.getMessage(),infoMessage);
    }
    virtual void recordProcessComplete()
    {
         message("recordProcessComplete",infoMessage);
         event.signal();
    }
private:
    MyProcessRequesterPtr getPtrSelf()
    {
        return shared_from_this();
    }

    MyProcessRequester(
        String const &requesterName,
        PVRecordPtr const & pvRecord)
    : isDetached(false),
      requesterName(requesterName),
      pvRecord(pvRecord)
    {}
    void init() {
        pvRecord->addPVRecordClient(getPtrSelf());
        pvRecord->addRequester(getPtrSelf());
    }
    
    Event event;
    bool isDetached;
    String requesterName;
    PVRecordPtr pvRecord;
};

class MyPutRequester :
   public virtual RecordPutRequester,
   public std::tr1::enable_shared_from_this<MyPutRequester>
{
public:
    MyPutRequester(PVRecordPtr const & pvRecord)
    : pvRecord(pvRecord),
      result(false)
    {}
    virtual ~MyPutRequester() {}
    virtual void requestResult(bool result)
    {
         this->result = result;
         event.signal();
    }
    bool makeRequest()
    {
        pvRecord->queuePutRequest(getPtrSelf());
        event.wait();
        return result;
    }
private:
    MyPutRequesterPtr getPtrSelf()
    {
        return shared_from_this();
    }
    Event event;
    PVRecordPtr pvRecord;
    bool result;
};



void dumpPVRecordField(PVRecordFieldPtr pvRecordField)
{
    PVRecordStructurePtr pvRecordStructure = pvRecordField->getParent();
    PVFieldPtr pvField = pvRecordField->getPVField();
    String fieldName = pvField->getFieldName();
    String fullFieldName = pvRecordField->getFullFieldName();
    String fullName = pvRecordField->getFullName();
    PVRecordPtr pvRecord = pvRecordField->getPVRecord();
    String recordName = pvRecord->getRecordName();
    printf("recordName %s fullName %s fullFieldName %s fieldName %s\n",
        recordName.c_str(),
        fullName.c_str(),
        fullFieldName.c_str(),
        fieldName.c_str());
}

void dumpPVRecordStructure(PVRecordStructurePtr pvRecordStructure)
{
    dumpPVRecordField(pvRecordStructure);
    PVRecordFieldPtrArrayPtr pvRecordFields =
        pvRecordStructure->getPVRecordFields();
    size_t num =  pvRecordFields->size();
    for(size_t i=0; i<num; i++) {
         PVRecordFieldPtr pvRecordField = (*pvRecordFields.get())[i];
         if(pvRecordField->getPVField()->getField()->getType()==structure) {
             PVRecordStructurePtr xxx =
                 static_pointer_cast<PVRecordStructure>(pvRecordField);
             dumpPVRecordStructure(xxx);
         } else {
             dumpPVRecordField(pvRecordField);
         }
    }
}

int main(int argc,char *argv[])
{
    String recordName("testRecord");
    PVRecordPtr pvRecord = TestRecord::create(recordName);
    dumpPVRecordStructure(pvRecord->getPVRecordStructure());
    PVStructurePtr pvStructure =
        pvRecord->getPVRecordStructure()->getPVStructure();
    PVLongPtr pvValue = pvStructure->getLongField("value");
    String builder;
    pvStructure->toString(&builder);
    printf("pvStructure\n%s\n",builder.c_str());
    builder.clear();
    pvValue->toString(&builder);
    printf("value\n%s\n",builder.c_str());
    PVRecordFieldPtr recordFieldValue = pvRecord->findPVRecordField(pvValue);
    MyPVListenerPtr listenTop = MyPVListener::create(
        "listenTop", pvRecord, pvRecord->getPVRecordStructure());
    MyPVListenerPtr listenValue = MyPVListener::create(
        "listenValue", pvRecord,recordFieldValue);

    PVFieldPtr pvDisplay = pvStructure->getSubField("display");
    PVRecordFieldPtr recordFieldDisplay =
        pvRecord->findPVRecordField(pvDisplay);
    MyPVListenerPtr listenDisplay = MyPVListener::create(
        "listenDisplay", pvRecord,recordFieldDisplay);

    PVStringPtr pvDisplayDescription =
        pvStructure->getStringField("display.description");
    PVRecordFieldPtr recordFieldDisplayDescription =
        pvRecord->findPVRecordField(pvDisplayDescription);
    MyPVListenerPtr listenDisplayDescription = MyPVListener::create(
        "listenDisplayDescription", pvRecord,recordFieldDisplayDescription);

    recordFieldDisplayDescription->message("test message",infoMessage);

    MyProcessRequesterPtr process1 =
        MyProcessRequester::create("process1",pvRecord);
    MyProcessRequesterPtr process2 =
        MyProcessRequester::create("process2",pvRecord);
    process1->process();
    builder.clear();
    pvValue->toString(&builder);
    printf("%s\n",builder.c_str());
    process2->process();
    builder.clear();
    pvValue->toString(&builder);
    printf("%s\n",builder.c_str());
    bool requestResult;
    requestResult = pvRecord->requestImmediatePut(pvValue);
    if(requestResult) {
        printf("error requestImmediatePut for pvValue returned true");
        pvRecord->immediatePutDone();
    }
    requestResult = pvRecord->requestImmediatePut(pvDisplayDescription);
    if(!requestResult) {
        printf("error requestImmediatePut for pvDisplayDescription returned false");
    } else {
        pvDisplayDescription->put("this is description");
        pvRecord->immediatePutDone();
    }

    MyPutRequesterPtr myPut(new  MyPutRequester(pvRecord));
    requestResult = myPut->makeRequest();
    if(!requestResult) {
        printf("error myPut->makeRequest() returned false");
    } else {
        pvDisplayDescription->put("this is new description");
        pvValue->put(1000);
        PVIntPtr pvSeverity = pvStructure->getIntField("alarm.severity");
        pvSeverity->put(3);
        PVIntPtr pvStatus = pvStructure->getIntField("alarm.status");
        pvStatus->put(2);
        PVStringPtr pvMessage = pvStructure->getStringField("alarm.message");
        pvMessage->put("alarmMessage");
        pvRecord->putDone(myPut);
    }
    
    builder.clear();
    pvStructure->toString(&builder);
    printf("pvStructure\n%s\n",builder.c_str());
    builder.clear();

    pvRecord->destroy();
    printf("all done\n");
#ifdef XXXXXX
    PVDatabasePtr pvDatabase = PVDatabase::getMaster();
    pvDatabase->addRecord(pvRecord);
    cout << recordName << "\n";
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;
        
    }
#endif
    return 0;
}

