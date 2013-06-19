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

#include <pv/channelProviderLocal.h>
#include <pv/convert.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
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
//         if(channelLocalTrace->getLevel()>0)
         {
             cout << "~ChannelProcessLocal() " << endl;
         }
    }
    static ChannelProcessLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace);
    virtual void process(bool lastRequest);
    virtual void destroy();
    virtual void lock()
    {
        thelock.lock();
        if(channelLocalTrace->getLevel()>2)
        {
            cout << "ChannelProcessLocal::lock";
            cout << " recordName " << pvRecord->getRecordName() << endl;
        }
    }
    virtual void unlock()
    {
        thelock.unlock();
        if(channelLocalTrace->getLevel()>2)
        {
            cout << "ChannelProcessLocal::unlock";
            cout << " recordName " << pvRecord->getRecordName() << endl;
        }
    }
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelProcessLocal(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace,
        int nProcess)
    : 
      isDestroyed(false),
      channelLocal(channelLocal),
      channelProcessRequester(channelProcessRequester),
      pvRecord(pvRecord),
      channelLocalTrace(channelLocalTrace),
      thelock(mutex),
      nProcess(nProcess)
    {
        thelock.unlock();
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelProcessRequester::shared_pointer channelProcessRequester,;
    PVRecordPtr pvRecord;
    ChannelLocalTracePtr channelLocalTrace;
    Mutex mutex;
    Lock thelock;
    int nProcess;
};

ChannelProcessLocalPtr ChannelProcessLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelProcessRequester::shared_pointer const & channelProcessRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord,
    ChannelLocalTracePtr const &channelLocalTrace)
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
        channelLocalTrace,
        nProcess));
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelProcessLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelLocal->addChannelProcess(process);
    channelProcessRequester->channelProcessConnect(Status::Ok, process);
    return process;
}


void ChannelProcessLocal::destroy()
{
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelProcessLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    if(isDestroyed) return;
    isDestroyed = true;
    channelLocal->removeChannelProcess(getPtrSelf());
    channelLocal.reset();
    channelProcessRequester.reset();
    pvRecord.reset();
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
    if(channelLocalTrace->getLevel()>1)
    {
        cout << "ChannelProcessLocal::process";
        cout << " nProcess " << nProcess << endl;
    }
    for(int i=0; i< nProcess; i++) {
        pvRecord->lock();
        pvRecord->beginGroupPut();
        pvRecord->process();
        pvRecord->endGroupPut();
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
//         if(channelLocalTrace->getLevel()>0)
        {
           cout << "~ChannelGetLocal()" << endl;
         }
    }
    static ChannelGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelGetRequester::shared_pointer const & channelGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace);
    virtual void get(bool lastRequest);
    virtual void destroy();
    virtual void lock() {thelock.lock();}
    virtual void unlock() {thelock.unlock();}
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
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace)
    : 
      firstTime(true),
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelGetRequester(channelGetRequester),
      pvCopy(pvCopy),
      pvStructure(pvStructure),
      bitSet(bitSet),
      pvRecord(pvRecord),
      channelLocalTrace(channelLocalTrace),
      thelock(mutex)
    {
        thelock.unlock();
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
    ChannelLocalTracePtr channelLocalTrace;
    Mutex mutex;
    Lock thelock;
};

ChannelGetLocalPtr ChannelGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelGetRequester::shared_pointer const & channelGetRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord,
    ChannelLocalTracePtr const &channelLocalTrace)
{
    PVCopyPtr pvCopy = PVCopy::create(
        pvRecord,
        pvRequest,
        "field");
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
        pvRecord,
        channelLocalTrace));
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelGetLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelLocal->addChannelGet(get);
    channelGetRequester->channelGetConnect(Status::Ok, get, pvStructure,bitSet);
    return get;
}


void ChannelGetLocal::destroy()
{
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelGetLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    if(isDestroyed) return;
    isDestroyed = true;
    channelLocal->removeChannelGet(getPtrSelf());
    channelLocal.reset();
    channelGetRequester.reset();
    pvCopy.reset();
    pvStructure.reset();
    bitSet.reset();
    pvRecord.reset();
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
    if(callProcess) {
        pvRecord->beginGroupPut();
        pvRecord->process();
        pvRecord->endGroupPut();
    }
    pvCopy->updateCopySetBitSet(pvStructure, bitSet, false);
    pvRecord->unlock();
    if(firstTime) {
        bitSet->clear();
        bitSet->set(0);
        firstTime = false;
    } 
    channelGetRequester->getDone(Status::Ok);
    if(channelLocalTrace->getLevel()>1)
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
//         if(channelLocalTrace->getLevel()>0)
         {
            cout << "~ChannelPutLocal()" << endl;
        }
    }
    static ChannelPutLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutRequester::shared_pointer const & channelPutRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace);
    virtual void put(bool lastRequest);
    virtual void get();
    virtual void destroy();
    virtual void lock() {thelock.lock();}
    virtual void unlock() {thelock.unlock();}
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
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace)
    :
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelPutRequester(channelPutRequester),
      pvCopy(pvCopy),
      pvStructure(pvStructure),
      bitSet(bitSet),
      pvRecord(pvRecord),
      channelLocalTrace(channelLocalTrace),
      thelock(mutex)
    {
        thelock.unlock();
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelPutRequester::shared_pointer channelPutRequester,;
    PVCopyPtr pvCopy;
    PVStructurePtr pvStructure;
    BitSetPtr bitSet;
    PVRecordPtr pvRecord;
    ChannelLocalTracePtr channelLocalTrace;
    Mutex mutex;
    Lock thelock;
};

ChannelPutLocalPtr ChannelPutLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelPutRequester::shared_pointer const & channelPutRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord,
    ChannelLocalTracePtr const &channelLocalTrace)
{
    PVCopyPtr pvCopy = PVCopy::create(
        pvRecord,
        pvRequest,
        "field");
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
        pvRecord,
        channelLocalTrace));
    channelLocal->addChannelPut(put);
    channelPutRequester->channelPutConnect(Status::Ok, put, pvStructure,bitSet);
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelPutLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    return put;
}

void ChannelPutLocal::destroy()
{
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelPutLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    if(isDestroyed) return;
    isDestroyed = true;
    channelLocal->removeChannelPut(getPtrSelf());
    channelLocal.reset();
    channelPutRequester.reset();
    pvCopy.reset();
    pvStructure.reset();
    bitSet.reset();
    pvRecord.reset();
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
    pvCopy->updateCopyFromBitSet(pvStructure, bitSet, false);
    pvRecord->unlock();
    channelPutRequester->getDone(Status::Ok);
    if(channelLocalTrace->getLevel()>1)
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
    pvRecord->beginGroupPut();
    pvCopy->updateRecord(pvStructure, bitSet, false);
    if(callProcess) {
         pvRecord->process();
    }
    pvRecord->endGroupPut();
    pvRecord->unlock();
    channelPutRequester->putDone(Status::Ok);
    if(channelLocalTrace->getLevel()>1)
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
//         if(channelLocalTrace->getLevel()>0)
         {
            cout << "~ChannelPutGetLocal()" << endl;
        }
    }
    static ChannelPutGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace);
    virtual void putGet(bool lastRequest);
    virtual void getPut();
    virtual void getGet();
    virtual void destroy();
    virtual void lock() {thelock.lock();}
    virtual void unlock() {thelock.unlock();}
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
        PVRecordPtr const &pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace)
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
      pvRecord(pvRecord),
      channelLocalTrace(channelLocalTrace),
      thelock(mutex)
    {
        thelock.unlock();
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
    ChannelLocalTracePtr channelLocalTrace;
    Mutex mutex;
    Lock thelock;
};

ChannelPutGetLocalPtr ChannelPutGetLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord,
    ChannelLocalTracePtr const &channelLocalTrace)
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
        pvRecord,
        channelLocalTrace));
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelPutGetLocal::create";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
    channelLocal->addChannelPutGet(putGet);
    channelPutGetRequester->channelPutGetConnect(
        Status::Ok, putGet, pvPutStructure,pvGetStructure);
    return putGet;
}


void ChannelPutGetLocal::destroy()
{
    if(channelLocalTrace->getLevel()>0)
    {
        cout << "ChannelPutGetLocal::destroy";
        cout << " destroyed " << isDestroyed << endl;
    }
    if(isDestroyed) return;
    isDestroyed = true;
    channelLocal->removeChannelPutGet(getPtrSelf());
    channelLocal.reset();
    channelPutGetRequester.reset();
    pvPutCopy.reset();
    pvGetCopy.reset();
    pvPutStructure.reset();
    pvGetStructure.reset();
    putBitSet.reset();
    getBitSet.reset();
    pvRecord.reset();
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
    pvRecord->beginGroupPut();
    pvPutCopy->updateRecord(pvPutStructure, putBitSet, false);
    if(callProcess) pvRecord->process();
    pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet, false);
    pvRecord->endGroupPut();
    pvRecord->unlock();
    getBitSet->clear();
    getBitSet->set(0);
    channelPutGetRequester->putGetDone(Status::Ok);
    if(channelLocalTrace->getLevel()>1)
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
    pvPutCopy->updateCopySetBitSet(pvPutStructure, putBitSet, false);
    pvRecord->unlock();
    putBitSet->clear();
    putBitSet->set(0);
    channelPutGetRequester->getPutDone(Status::Ok);
    if(channelLocalTrace->getLevel()>1)
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
    pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet, false);
    pvRecord->unlock();
    getBitSet->clear();
    getBitSet->set(0);
    channelPutGetRequester->getGetDone(Status::Ok);
    if(channelLocalTrace->getLevel()>1)
    {
        cout << "ChannelPutGetLocal::getGet" << endl;
    }
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
    PVRecordPtr const & pvRecord,
    ChannelLocalTracePtr const &channelLocalTrace)
:   provider(provider),
    requester(requester),
    pvRecord(pvRecord),
    channelLocalTrace(channelLocalTrace),
    beingDestroyed(false)
{
}

ChannelLocal::~ChannelLocal()
{
//    if(channelLocalTrace->getLevel()>0)
    {
        cout << "~ChannelLocal()" << endl;
    }
}

void ChannelLocal::destroy()
{
    if(channelLocalTrace->getLevel()>0) {
         cout << "ChannelLocal::destroy() ";
         cout << "beingDestroyed " << beingDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    provider->removeChannel(getPtrSelf());
    pvRecord->removePVRecordClient(getPtrSelf());
    while(true) {
        std::multiset<ChannelProcess::shared_pointer>::iterator it;
        it = channelProcessList.begin();
        if(it==channelProcessList.end()) break;
        if(channelLocalTrace->getLevel()>2) {
             cout << "ChannelLocal destroying channelProcess " << endl;
        }
        it->get()->destroy();
        channelProcessList.erase(it);
    }
    while(true) {
        std::multiset<ChannelGet::shared_pointer>::iterator it;
        it = channelGetList.begin();
        if(it==channelGetList.end()) break;
        if(channelLocalTrace->getLevel()>2) {
             cout << "ChannelLocal destroying channelGet " << endl;
        }
        it->get()->destroy();
        channelGetList.erase(it);
    }
    while(true) {
        std::multiset<ChannelPut::shared_pointer>::iterator it;
        it = channelPutList.begin();
        if(it==channelPutList.end()) break;
        if(channelLocalTrace->getLevel()>2) {
             cout << "ChannelLocal destroying channelPut " << endl;
        }
        it->get()->destroy();
        channelPutList.erase(it);
    }
    while(true) {
        std::multiset<ChannelPutGet::shared_pointer>::iterator it;
        it = channelPutGetList.begin();
        if(it==channelPutGetList.end()) break;
        if(channelLocalTrace->getLevel()>2) {
             cout << "ChannelLocal destroying channelPutGet " << endl;
        }
        it->get()->destroy();
        channelPutGetList.erase(it);
    }
    while(true) {
        std::multiset<ChannelRPC::shared_pointer>::iterator it;
        it = channelRPCList.begin();
        if(it==channelRPCList.end()) break;
        if(channelLocalTrace->getLevel()>2) {
             cout << "ChannelLocal destroying channelRPC " << endl;
        }
        it->get()->destroy();
        channelRPCList.erase(it);
    }
    while(true) {
        std::multiset<ChannelArray::shared_pointer>::iterator it;
        it = channelArrayList.begin();
        if(it==channelArrayList.end()) break;
        if(channelLocalTrace->getLevel()>2) {
             cout << "ChannelLocal destroying channelArray " << endl;
        }
        it->get()->destroy();
        channelArrayList.erase(it);
    }
}

void ChannelLocal::detach(PVRecordPtr const & pvRecord)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::detach() " << endl;
    }
    destroy();
}

void ChannelLocal::addChannelProcess(ChannelProcess::shared_pointer const & channelProcess)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::addChannelProcess() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelProcessList.insert(channelProcess);
}

void ChannelLocal::addChannelGet(ChannelGet::shared_pointer const &channelGet)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::addChannelGet() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelGetList.insert(channelGet);
}

void ChannelLocal::addChannelPut(ChannelPut::shared_pointer const &channelPut)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::addChannelPut() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelPutList.insert(channelPut);
}

void ChannelLocal::addChannelPutGet(ChannelPutGet::shared_pointer const &channelPutGet)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::addChannelPutGet() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelPutGetList.insert(channelPutGet);
}

void ChannelLocal::addChannelRPC(ChannelRPC::shared_pointer const &channelRPC)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::addChannelRPC() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelRPCList.insert(channelRPC);
}

void ChannelLocal::addChannelArray(ChannelArray::shared_pointer const &channelArray)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::addChannelArray() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelArrayList.insert(channelArray);
}

void ChannelLocal::removeChannelProcess(ChannelProcess::shared_pointer const &ref)
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::removeChannelProcess() " << endl;
    }
    channelProcessList.erase(ref);
}

void ChannelLocal::removeChannelGet(ChannelGet::shared_pointer const &ref)
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::removeChannelGet() " << endl;
    }
    channelGetList.erase(ref);
}

void ChannelLocal::removeChannelPut(ChannelPut::shared_pointer const &ref)
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::removeChannelPut() " << endl;
    }
    channelPutList.erase(ref);
}

void ChannelLocal::removeChannelPutGet(ChannelPutGet::shared_pointer const &ref)
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::removeChannelPutGet() " << endl;
    }
    channelPutGetList.erase(ref);
}

void ChannelLocal::removeChannelRPC(ChannelRPC::shared_pointer const &ref)
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::removeChannelRPC() " << endl;
    }
    channelRPCList.erase(ref);
}

void ChannelLocal::removeChannelArray(ChannelArray::shared_pointer const &ref)
{
    if(channelLocalTrace->getLevel()>1) {
         cout << "ChannelLocal::removeChannelArray() " << endl;
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelArrayList.erase(ref);
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
            pvRecord,
            channelLocalTrace);
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
            pvRecord,
            channelLocalTrace);
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
            pvRecord,
            channelLocalTrace);
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
            pvRecord,
            channelLocalTrace);
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
            pvRequest,
            channelLocalTrace);
    return monitor;
}

ChannelArray::shared_pointer ChannelLocal::createChannelArray(
        ChannelArrayRequester::shared_pointer const &channelArrayRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelArray not supported"));
    channelArrayRequester->channelArrayConnect(
        status,
        ChannelArray::shared_pointer(),
        PVArray::shared_pointer());
    return ChannelArray::shared_pointer();
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
