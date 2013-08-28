/* channelLocal.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */

#include <sstream>

#include <epicsThread.h>

#include <pv/timeStamp.h>
#include <pv/channelProviderLocal.h>
#include <pv/convert.h>
#include <pv/pvSubArrayCopy.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;

static ConvertPtr convert = getConvert();

class ChannelProcessLocal;
typedef std::tr1::shared_ptr<ChannelProcessLocal> ChannelProcessLocalPtr;
class ChannelGetLocal;
typedef std::tr1::shared_ptr<ChannelGetLocal> ChannelGetLocalPtr;
class ChannelPutLocal;
typedef std::tr1::shared_ptr<ChannelPutLocal> ChannelPutLocalPtr;
class ChannelPutGetLocal;
typedef std::tr1::shared_ptr<ChannelPutGetLocal> ChannelPutGetLocalPtr;
class ChannelMonitorLocal;
typedef std::tr1::shared_ptr<ChannelMonitorLocal> ChannelMonitorLocalPtr;
class ChannelRPCLocal;
typedef std::tr1::shared_ptr<ChannelRPCLocal> ChannelRPCLocalPtr;
class ChannelArrayLocal;
typedef std::tr1::shared_ptr<ChannelArrayLocal> ChannelArrayLocalPtr;

static bool getProcess(PVStructurePtr pvRequest,bool processDefault)
{
    PVFieldPtr pvField = pvRequest->getSubField("record._options.process");
    if(pvField.get()==NULL || pvField->getField()->getType()!=scalar) {
        return processDefault;
    }
    ScalarConstPtr scalar = static_pointer_cast<const Scalar>(
        pvField->getField());
    if(scalar->getScalarType()==pvString) {
        PVStringPtr pvString = static_pointer_cast<PVString>(pvField);
        return  pvString->get().compare("true")==0 ? true : false;
    } else if(scalar->getScalarType()==pvBoolean) {
        PVBooleanPtr pvBoolean = static_pointer_cast<PVBoolean>(pvField);
       	return pvBoolean.get();
    }
    return processDefault;
}

class ChannelProcessLocal :
    public ChannelProcess,
    public std::tr1::enable_shared_from_this<ChannelProcessLocal>
{
public:
    POINTER_DEFINITIONS(ChannelProcessLocal);
    virtual ~ChannelProcessLocal()
    {
         if(pvRecord->getTraceLevel()>0)
         {
             cout << "~ChannelProcessLocal() " << endl;
         }
    }
    static ChannelProcessLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void process(bool lastRequest);
    virtual void destroy();
    virtual void lock() {mutex.lock();}
    virtual void unlock() {mutex.unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelProcessLocal(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVRecordPtr const &pvRecord,
        int nProcess)
    : 
      isDestroyed(false),
      channelLocal(channelLocal),
      channelProcessRequester(channelProcessRequester),
      pvRecord(pvRecord),
      nProcess(nProcess)
    {
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelProcessRequester::shared_pointer channelProcessRequester,;
    PVRecordPtr pvRecord;
    Mutex mutex;
    int nProcess;
};

ChannelProcessLocalPtr ChannelProcessLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelProcessRequester::shared_pointer const & channelProcessRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVFieldPtr pvField;
    PVStructurePtr pvOptions;
    int nProcess = 1;
    if(pvRequest!=NULL) pvField = pvRequest->getSubField("record._options");
    if(pvField.get()!=NULL) {
        pvOptions = static_pointer_cast<PVStructure>(pvField);
        pvField = pvOptions->getSubField("nProcess");
        if(pvField.get()!=NULL) {
            PVStringPtr pvString = pvOptions->getStringField("nProcess");
            if(pvString.get()!=NULL) {
                int size;
                std::stringstream ss;
                ss << pvString->get();
                ss >> size;
                nProcess = size;
            }
        }
    }
    ChannelProcessLocalPtr process(new ChannelProcessLocal(
        channelLocal,
        channelProcessRequester,
        pvRecord,
        nProcess));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelProcessLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelProcessRequester->channelProcessConnect(Status::Ok, process);
    return process;
}


void ChannelProcessLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelProcessLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    channelLocal.reset();
}

void ChannelProcessLocal::process(bool lastRequest)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelProcessRequester->processDone(status);
         return;
    } 
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelProcessLocal::process";
        cout << " nProcess " << nProcess << endl;
    }
    for(int i=0; i< nProcess; i++) {
        pvRecord->lock();
        try {
            pvRecord->beginGroupPut();
            pvRecord->process();
            pvRecord->endGroupPut();
        } catch(...) {
            pvRecord->unlock();
            throw;
        }
        pvRecord->unlock();
    }
    channelProcessRequester->processDone(Status::Ok);
    if(lastRequest) destroy();
}

class ChannelGetLocal :
    public ChannelGet,
    public std::tr1::enable_shared_from_this<ChannelGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelGetLocal);
    virtual ~ChannelGetLocal()
    {
         if(pvRecord->getTraceLevel()>0)
        {
           cout << "~ChannelGetLocal()" << endl;
         }
    }
    static ChannelGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelGetRequester::shared_pointer const & channelGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void get(bool lastRequest);
    virtual void destroy();
    virtual void lock() {mutex.lock();}
    virtual void unlock() {mutex.unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelGetLocal(
        bool callProcess,
        ChannelLocalPtr const &channelLocal,
        ChannelGetRequester::shared_pointer const & channelGetRequester,
        PVCopyPtr const &pvCopy,
        PVStructurePtr const&pvStructure,
        BitSetPtr const & bitSet,
        PVRecordPtr const &pvRecord)
    : 
      firstTime(true),
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelGetRequester(channelGetRequester),
      pvCopy(pvCopy),
      pvStructure(pvStructure),
      bitSet(bitSet),
      pvRecord(pvRecord)
    {
    }
    bool firstTime;
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelGetRequester::shared_pointer channelGetRequester,;
    PVCopyPtr pvCopy;
    PVStructurePtr pvStructure;
    BitSetPtr bitSet;
    PVRecordPtr pvRecord;
    Mutex mutex;
};

ChannelGetLocalPtr ChannelGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelGetRequester::shared_pointer const & channelGetRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVCopyPtr pvCopy = PVCopy::create(
        pvRecord,
        pvRequest,
        "");
    if(pvCopy.get()==NULL) {
        Status status(
            Status::Status::STATUSTYPE_ERROR,
            "invalid pvRequest");
        ChannelGet::shared_pointer channelGet;
        PVStructurePtr pvStructure;
        BitSetPtr bitSet;
        channelGetRequester->channelGetConnect(
            status,
            channelGet,
            pvStructure,
            bitSet);
        ChannelGetLocalPtr localGet;
        return localGet;
    }
    PVStructurePtr pvStructure = pvCopy->createPVStructure();
    BitSetPtr   bitSet(new BitSet(pvStructure->getNumberFields()));
    ChannelGetLocalPtr get(new ChannelGetLocal(
        getProcess(pvRequest,false),
        channelLocal,
        channelGetRequester,
        pvCopy,
        pvStructure,
        bitSet,
        pvRecord));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelGetLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelGetRequester->channelGetConnect(Status::Ok, get, pvStructure,bitSet);
    return get;
}


void ChannelGetLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelGetLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    channelLocal.reset();
}

void ChannelGetLocal::get(bool lastRequest)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelGetRequester->getDone(status);
         return;
    } 
    bitSet->clear();
    pvRecord->lock();
    try {
        if(callProcess) {
            pvRecord->beginGroupPut();
            pvRecord->process();
            pvRecord->endGroupPut();
        }
        pvCopy->updateCopySetBitSet(pvStructure, bitSet);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    if(firstTime) {
        bitSet->clear();
        bitSet->set(0);
        firstTime = false;
    } 
    channelGetRequester->getDone(Status::Ok);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelGetLocal::get" << endl;
    }
    if(lastRequest) destroy();
}

class ChannelPutLocal :
    public ChannelPut,
    public std::tr1::enable_shared_from_this<ChannelPutLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutLocal);
    virtual ~ChannelPutLocal()
    {
         if(pvRecord->getTraceLevel()>0)
         {
            cout << "~ChannelPutLocal()" << endl;
        }
    }
    static ChannelPutLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutRequester::shared_pointer const & channelPutRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void put(bool lastRequest);
    virtual void get();
    virtual void destroy();
    virtual void lock() {mutex.lock();}
    virtual void unlock() {mutex.unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelPutLocal(
        bool callProcess,
        ChannelLocalPtr const &channelLocal,
        ChannelPutRequester::shared_pointer const & channelPutRequester,
        PVCopyPtr const &pvCopy,
        PVStructurePtr const&pvStructure,
        BitSetPtr const & bitSet,
        PVRecordPtr const &pvRecord)
    :
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelPutRequester(channelPutRequester),
      pvCopy(pvCopy),
      pvStructure(pvStructure),
      bitSet(bitSet),
      pvRecord(pvRecord)
    {
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelPutRequester::shared_pointer channelPutRequester,;
    PVCopyPtr pvCopy;
    PVStructurePtr pvStructure;
    BitSetPtr bitSet;
    PVRecordPtr pvRecord;
    Mutex mutex;
};

ChannelPutLocalPtr ChannelPutLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelPutRequester::shared_pointer const & channelPutRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVCopyPtr pvCopy = PVCopy::create(
        pvRecord,
        pvRequest,
        "");
    if(pvCopy.get()==NULL) {
        Status status(
            Status::Status::STATUSTYPE_ERROR,
            "invalid pvRequest");
        ChannelPut::shared_pointer channelPut;
        PVStructurePtr pvStructure;
        BitSetPtr bitSet;
        channelPutRequester->channelPutConnect(
            status,
            channelPut,
            pvStructure,
            bitSet);
        ChannelPutLocalPtr localPut;
        return localPut;
    }
    PVStructurePtr pvStructure = pvCopy->createPVStructure();
    BitSetPtr   bitSet(new BitSet(pvStructure->getNumberFields()));
    ChannelPutLocalPtr put(new ChannelPutLocal(
        getProcess(pvRequest,true),
        channelLocal,
        channelPutRequester,
        pvCopy,
        pvStructure,
        bitSet,
        pvRecord));
    channelPutRequester->channelPutConnect(Status::Ok, put, pvStructure,bitSet);
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelPutLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    return put;
}

void ChannelPutLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelPutLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    channelLocal.reset();
}

void ChannelPutLocal::get()
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutRequester->getDone(status);
         return;
    }
    bitSet->clear();
    bitSet->set(0);
    pvRecord->lock();
    try {
        pvCopy->updateCopyFromBitSet(pvStructure, bitSet);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    channelPutRequester->getDone(Status::Ok);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelPutLocal::get" << endl;
    }
}

void ChannelPutLocal::put(bool lastRequest)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutRequester->getDone(status);
         return;
    }
    pvRecord->lock();
    try {
        pvRecord->beginGroupPut();
        pvCopy->updateRecord(pvStructure, bitSet);
        if(callProcess) {
             pvRecord->process();
        }
        pvRecord->endGroupPut();
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    channelPutRequester->putDone(Status::Ok);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelPutLocal::put" << endl;
    }
    if(lastRequest) destroy();
}


class ChannelPutGetLocal :
    public ChannelPutGet,
    public std::tr1::enable_shared_from_this<ChannelPutGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutGetLocal);
    virtual ~ChannelPutGetLocal()
    {
         if(pvRecord->getTraceLevel()>0)
         {
            cout << "~ChannelPutGetLocal()" << endl;
        }
    }
    static ChannelPutGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void putGet(bool lastRequest);
    virtual void getPut();
    virtual void getGet();
    virtual void destroy();
    virtual void lock() {mutex.lock();}
    virtual void unlock() {mutex.unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelPutGetLocal(
        bool callProcess,
        ChannelLocalPtr const &channelLocal,
        ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
        PVCopyPtr const &pvPutCopy,
        PVCopyPtr const &pvGetCopy,
        PVStructurePtr const&pvPutStructure,
        PVStructurePtr const&pvGetStructure,
        BitSetPtr const & putBitSet,
        BitSetPtr const & getBitSet,
        PVRecordPtr const &pvRecord)
    : 
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelPutGetRequester(channelPutGetRequester),
      pvPutCopy(pvPutCopy),
      pvGetCopy(pvGetCopy),
      pvPutStructure(pvPutStructure),
      pvGetStructure(pvGetStructure),
      putBitSet(putBitSet),
      getBitSet(getBitSet),
      pvRecord(pvRecord)
    {
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelPutGetRequester::shared_pointer channelPutGetRequester,;
    PVCopyPtr pvPutCopy;
    PVCopyPtr pvGetCopy;
    PVStructurePtr pvPutStructure;
    PVStructurePtr pvGetStructure;
    BitSetPtr putBitSet;
    BitSetPtr getBitSet;
    PVRecordPtr pvRecord;
    Mutex mutex;
};

ChannelPutGetLocalPtr ChannelPutGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVCopyPtr pvPutCopy = PVCopy::create(
        pvRecord,
        pvRequest,
        "putField");
    PVCopyPtr pvGetCopy = PVCopy::create(
        pvRecord,
        pvRequest,
        "getField");
    if(pvPutCopy.get()==NULL || pvGetCopy.get()==NULL) {
        Status status(
            Status::Status::STATUSTYPE_ERROR,
            "invalid pvRequest");
        ChannelPutGet::shared_pointer channelPutGet;
        PVStructurePtr pvStructure;
        BitSetPtr bitSet;
        channelPutGetRequester->channelPutGetConnect(
            status,
            channelPutGet,
            pvStructure,
            pvStructure);
        ChannelPutGetLocalPtr localPutGet;
        return localPutGet;
    }
    PVStructurePtr pvPutStructure = pvPutCopy->createPVStructure();
    PVStructurePtr pvGetStructure = pvGetCopy->createPVStructure();
    BitSetPtr   putBitSet(new BitSet(pvPutStructure->getNumberFields()));
    BitSetPtr   getBitSet(new BitSet(pvGetStructure->getNumberFields()));
    pvRecord->lock();
    try {
        pvPutCopy->initCopy(pvPutStructure,putBitSet);
        pvGetCopy->initCopy(pvGetStructure,getBitSet);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    ChannelPutGetLocalPtr putGet(new ChannelPutGetLocal(
        getProcess(pvRequest,true),
        channelLocal,
        channelPutGetRequester,
        pvPutCopy,
        pvGetCopy,
        pvPutStructure,
        pvGetStructure,
        putBitSet,
        getBitSet,
        pvRecord));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelPutGetLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelPutGetRequester->channelPutGetConnect(
        Status::Ok, putGet, pvPutStructure,pvGetStructure);
    return putGet;
}


void ChannelPutGetLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelPutGetLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    channelLocal.reset();
}

void ChannelPutGetLocal::putGet(bool lastRequest)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutGetRequester->putGetDone(status);
         return;
    } 
    putBitSet->clear();
    putBitSet->set(0);
    pvRecord->lock();
    try {
        pvRecord->beginGroupPut();
        pvPutCopy->updateRecord(pvPutStructure, putBitSet);
        if(callProcess) pvRecord->process();
        pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet);
        pvRecord->endGroupPut();
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    getBitSet->clear();
    getBitSet->set(0);
    channelPutGetRequester->putGetDone(Status::Ok);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelPutGetLocal::putGet" << endl;
    }
    if(lastRequest) destroy();
}

void ChannelPutGetLocal::getPut()
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutGetRequester->getPutDone(status);
         return;
    } 
    pvRecord->lock();
    try {
        pvPutCopy->updateCopySetBitSet(pvPutStructure, putBitSet);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    putBitSet->clear();
    putBitSet->set(0);
    channelPutGetRequester->getPutDone(Status::Ok);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelPutGetLocal::getPut" << endl;
    }
}

void ChannelPutGetLocal::getGet()
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutGetRequester->getGetDone(status);
         return;
    } 
    pvRecord->lock();
    try {
        pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    getBitSet->clear();
    getBitSet->set(0);
    channelPutGetRequester->getGetDone(Status::Ok);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "ChannelPutGetLocal::getGet" << endl;
    }
}

typedef std::tr1::shared_ptr<PVArray> PVArrayPtr;

class ChannelArrayLocal :
    public ChannelArray,
    public std::tr1::enable_shared_from_this<ChannelArrayLocal>
{
public:
    POINTER_DEFINITIONS(ChannelArrayLocal);
    virtual ~ChannelArrayLocal()
    {
         if(pvRecord->getTraceLevel()>0)
         {
            cout << "~ChannelArrayLocal()" << endl;
        }
    }
    static ChannelArrayLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelArrayRequester::shared_pointer const & channelArrayRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void getArray(bool lastRequest,int offset, int count);
    virtual void putArray(bool lastRequest,int offset, int count);
    virtual void setLength(bool lastRequest,int length, int capacity);
    virtual void destroy();
    virtual void lock() {mutex.lock();}
    virtual void unlock() {mutex.unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelArrayLocal(
        ChannelLocalPtr const &channelLocal,
        ChannelArrayRequester::shared_pointer const & channelArrayRequester,
        PVArrayPtr const &pvArray,
        PVArrayPtr const &pvCopy,
        PVRecordPtr const &pvRecord)
    : 
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelArrayRequester(channelArrayRequester),
      pvArray(pvArray),
      pvCopy(pvCopy),
      pvRecord(pvRecord)
    {
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelArrayRequester::shared_pointer channelArrayRequester,;
    PVArrayPtr pvArray;
    PVArrayPtr pvCopy;
    PVRecordPtr pvRecord;
    Mutex mutex;
};


ChannelArrayLocalPtr ChannelArrayLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelArrayRequester::shared_pointer const & channelArrayRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    PVFieldPtrArray const & pvFields = pvRequest->getPVFields();
    if(pvFields.size()!=1) {
        Status status(
            Status::Status::STATUSTYPE_ERROR,"invalid pvRequest");
        ChannelArrayLocalPtr channelArray;
        PVScalarArrayPtr pvArray;
        channelArrayRequester->channelArrayConnect(status,channelArray,pvArray);
        return channelArray;
    }
    PVFieldPtr pvField = pvFields[0];
    String fieldName("");
    while(true) {
        String name = pvField->getFieldName();
        if(fieldName.size()>0) fieldName += '.';
        fieldName += name;
        PVStructurePtr pvs = static_pointer_cast<PVStructure>(pvField);
        PVFieldPtrArray const & pvfs = pvs->getPVFields();
        if(pvfs.size()!=1) break;
        pvField = pvfs[0];
    }
    pvField = pvRecord->getPVRecordStructure()->getPVStructure()->getSubField(fieldName);
    if(pvField==NULL) {
        Status status(
            Status::Status::STATUSTYPE_ERROR,fieldName +" not found");
        ChannelArrayLocalPtr channelArray;
        PVScalarArrayPtr pvArray;
        channelArrayRequester->channelArrayConnect(status,channelArray,pvArray);
        return channelArray;
    }
    if(pvField->getField()->getType()!=scalarArray && pvField->getField()->getType()!=structureArray)
    {
        Status status(
            Status::Status::STATUSTYPE_ERROR,fieldName +" not array");
        ChannelArrayLocalPtr channelArray;
        PVArrayPtr pvArray;
        channelArrayRequester->channelArrayConnect(status,channelArray,pvArray);
        return channelArray;
    }
    PVArrayPtr pvArray = static_pointer_cast<PVArray>(pvField);
    PVArrayPtr pvCopy;
    if(pvField->getField()->getType()==scalarArray) {
        PVScalarArrayPtr xxx = static_pointer_cast<PVScalarArray>(pvField);
        pvCopy = getPVDataCreate()->createPVScalarArray(
            xxx->getScalarArray()->getElementType());
    } else {
        PVStructureArrayPtr xxx = static_pointer_cast<PVStructureArray>(pvField);
        pvCopy = getPVDataCreate()->createPVStructureArray(
            xxx->getStructureArray());
    }
    
    ChannelArrayLocalPtr array(new ChannelArrayLocal(
        channelLocal,
        channelArrayRequester,
        pvArray,
        pvCopy,
        pvRecord));
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelArrayLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelArrayRequester->channelArrayConnect(
        Status::Ok, array, pvCopy);
    return array;
}


void ChannelArrayLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "ChannelArrayLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    channelLocal.reset();
}

void ChannelArrayLocal::getArray(bool lastRequest,int offset, int count)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelArrayRequester->getArrayDone(status);
         return;
    }
    if(pvRecord->getTraceLevel()>1)
    {
       cout << "ChannelArrayLocal::getArray" << endl;
    }
    pvRecord->lock();
    try {
        if(count<0) count = pvArray->getLength();
        size_t capacity = pvArray->getCapacity();
        if(capacity!=0) {
            pvCopy->setCapacity(capacity);
            pvCopy->setLength(count);
            copy(*pvArray.get(),offset,*pvCopy.get(),0,count);
        }
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    channelArrayRequester->getArrayDone(Status::Ok);
    if(lastRequest) destroy();
}

void ChannelArrayLocal::putArray(bool lastRequest,int offset, int count)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelArrayRequester->putArrayDone(status);
         return;
    }
    if(pvRecord->getTraceLevel()>1)
    {
       cout << "ChannelArrayLocal::putArray" << endl;
    }
    pvRecord->lock();
    try {
        if(count<=0) count = pvCopy->getLength();
        if(pvArray->getCapacity()<count) pvArray->setCapacity(count);
        if(pvArray->getLength()<count) pvArray->setLength(count);
        copy(*pvCopy.get(),0,*pvArray.get(),offset,count);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    channelArrayRequester->putArrayDone(Status::Ok);
    if(lastRequest) destroy();
}

void ChannelArrayLocal::setLength(bool lastRequest,int length, int capacity)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelArrayRequester->setLengthDone(status);
         return;
    }
    if(pvRecord->getTraceLevel()>1)
    {
       cout << "ChannelArrayLocal::setLength" << endl;
    }
    pvRecord->lock();
    try {
        if(capacity>=0 && !pvArray->isCapacityMutable()) {
             Status status(
                 Status::Status::STATUSTYPE_ERROR,
                "capacityImnutable");
             channelArrayRequester->setLengthDone(status);
             pvRecord->unlock();
             return;
        }
        if(capacity>0) {
            if(pvArray->getCapacity()!=capacity) pvArray->setCapacity(capacity);
        }
        if(length>0) {
            if(pvArray->getLength()!=length) pvArray->setLength(length);
        }
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
    channelArrayRequester->setLengthDone(Status::Ok);
}


class ChannelRPCLocal :
    public ChannelRPC
{
public:
    POINTER_DEFINITIONS(ChannelRPCLocal);
    virtual ~ChannelRPCLocal();
    static ChannelRPC::shared_pointer create(
        ChannelProviderLocalPtr const &channelProvider,
        ChannelRPC::shared_pointer const & channelRPCRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void request(
        PVStructurePtr const & pvArgument,
        bool lastRequest);
};


ChannelLocal::ChannelLocal(
    ChannelProviderLocalPtr const & provider,
    ChannelRequester::shared_pointer const & requester,
    PVRecordPtr const & pvRecord)
:   provider(provider),
    requester(requester),
    pvRecord(pvRecord),
    beingDestroyed(false)
{
}

ChannelLocal::~ChannelLocal()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "~ChannelLocal()" << endl;
    }
}

void ChannelLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0) {
         cout << "ChannelLocal::destroy() ";
         cout << "beingDestroyed " << beingDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    pvRecord->removePVRecordClient(getPtrSelf());
}

void ChannelLocal::detach(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>0) {
         cout << "ChannelLocal::detach() " << endl;
    }
    destroy();
}


String ChannelLocal::getRequesterName()
{
    return requester->getRequesterName();
}

void ChannelLocal::message(
        String const &message,
        MessageType messageType)
{
    requester->message(message,messageType);
}

String ChannelLocal::getRemoteAddress()
{
    return String("local");
}

Channel::ConnectionState ChannelLocal::getConnectionState()
{
    return Channel::CONNECTED;
}

String ChannelLocal::getChannelName()
{
    return pvRecord->getRecordName();
}

ChannelRequester::shared_pointer ChannelLocal::getChannelRequester()
{
    return requester;
}

bool ChannelLocal::isConnected()
{
    return true;
}

void ChannelLocal::getField(GetFieldRequester::shared_pointer const &requester,
        String const &subField)
{
    if(subField.size()<1) {
        StructureConstPtr structure =
            pvRecord->getPVRecordStructure()->getPVStructure()->getStructure();
        requester->getDone(Status::Ok,structure);
        return;
    } 
    PVFieldPtr pvField = 
        pvRecord->getPVRecordStructure()->getPVStructure()->getSubField(subField);
    if(pvField.get()!=NULL) {
        requester->getDone(Status::Ok,pvField->getField());
        return;
    }
    Status status(Status::STATUSTYPE_ERROR,
        String("client asked for illegal field"));
    requester->getDone(status,FieldConstPtr());
}

AccessRights ChannelLocal::getAccessRights(
        PVField::shared_pointer const &pvField)
{
    throw std::logic_error(String("Not Implemented"));
}

ChannelProcess::shared_pointer ChannelLocal::createChannelProcess(
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    ChannelProcessLocalPtr channelProcess =
       ChannelProcessLocal::create(
            getPtrSelf(),
            channelProcessRequester,
            pvRequest,
            pvRecord);
    return channelProcess;
}

ChannelGet::shared_pointer ChannelLocal::createChannelGet(
        ChannelGetRequester::shared_pointer const &channelGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    ChannelGetLocalPtr channelGet =
       ChannelGetLocal::create(
            getPtrSelf(),
            channelGetRequester,
            pvRequest,
            pvRecord);
    return channelGet;
}

ChannelPut::shared_pointer ChannelLocal::createChannelPut(
        ChannelPutRequester::shared_pointer const &channelPutRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    ChannelPutLocalPtr channelPut =
       ChannelPutLocal::create(
            getPtrSelf(),
            channelPutRequester,
            pvRequest,
            pvRecord);
    return channelPut;
}

ChannelPutGet::shared_pointer ChannelLocal::createChannelPutGet(
        ChannelPutGetRequester::shared_pointer const &channelPutGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    ChannelPutGetLocalPtr channelPutGet =
       ChannelPutGetLocal::create(
            getPtrSelf(),
            channelPutGetRequester,
            pvRequest,
            pvRecord);
    return channelPutGet;
}

ChannelRPC::shared_pointer ChannelLocal::createChannelRPC(
        ChannelRPCRequester::shared_pointer const & channelRPCRequester,
        PVStructure::shared_pointer const & pvRequest)
{
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelRPC not supported"));
    channelRPCRequester->channelRPCConnect(status,ChannelRPC::shared_pointer());
    return ChannelRPC::shared_pointer();
}

Monitor::shared_pointer ChannelLocal::createMonitor(
        MonitorRequester::shared_pointer const &monitorRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    MonitorPtr monitor = 
        getMonitorFactory()->createMonitor(
            pvRecord,
            monitorRequester,
            pvRequest);
    return monitor;
}

ChannelArray::shared_pointer ChannelLocal::createChannelArray(
        ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    ChannelArrayLocalPtr channelArray =
       ChannelArrayLocal::create(
            getPtrSelf(),
            channelArrayRequester,
            pvRequest,
            pvRecord);
    return channelArray;
}

void ChannelLocal::printInfo()
{
    cout << "ChannelLocal provides access to service" << endl;
}

void ChannelLocal::printInfo(StringBuilder out)
{
    *out += "ChannelLocal provides access to service";
}

}}
