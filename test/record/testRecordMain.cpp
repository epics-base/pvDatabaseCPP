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
   public PVRecordClient,
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
    PVFieldPtr pvValueAlarm = pvStructure->getSubField("valueAlarm");
    PVRecordFieldPtr recordFieldValueAlarm =
        pvRecord->findPVRecordField(pvValueAlarm);
    MyPVListenerPtr listenValueAlarm = MyPVListener::create(
        "listenValueAlarm", pvRecord,recordFieldValueAlarm);
    PVIntPtr pvHighAlarmSeverity =
        pvStructure->getIntField("valueAlarm.highAlarmSeverity");
    PVRecordFieldPtr recordFieldHighAlarmSeverity =
        pvRecord->findPVRecordField(pvHighAlarmSeverity);
    MyPVListenerPtr listenHighAlarmSeverity = MyPVListener::create(
        "listenHighAlarmSeverity", pvRecord,recordFieldHighAlarmSeverity);
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
    pvHighAlarmSeverity->put(3);
    recordFieldHighAlarmSeverity->message("test message",infoMessage);
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

