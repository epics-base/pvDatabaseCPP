/* channelLocal.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

#include <pv/channelProviderLocal.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;

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

class ChannelLocalDebug {
public:
    static void setLevel(int level);
    static int getLevel();
};

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
    virtual ~ChannelProcessLocal() {destroy();}
    static ChannelProcessLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void process(bool lastRequest);
    virtual void destroy();
    virtual void lock() {thelock.lock();}
    virtual void unlock() {thelock.unlock();}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelProcessLocal(
        ChannelLocalPtr const &channelLocal,
        ChannelProcessRequester::shared_pointer const & channelProcessRequester,
        PVRecordPtr const &pvRecord)
    : 
      isDestroyed(false),
      channelLocal(channelLocal),
      channelProcessRequester(channelProcessRequester),
      pvRecord(pvRecord),
      thelock(mutex)
    {
        thelock.unlock();
    }
    bool isDestroyed;
    bool callProcess;
    ChannelLocalPtr channelLocal;
    ChannelProcessRequester::shared_pointer channelProcessRequester,;
    PVRecordPtr pvRecord;
    Mutex mutex;
    Lock thelock;
};

ChannelProcessLocalPtr ChannelProcessLocal::create(
    ChannelLocalPtr const &channelLocal,
    ChannelProcessRequester::shared_pointer const & channelProcessRequester,
    PVStructurePtr const & pvRequest,
    PVRecordPtr const &pvRecord)
{
    ChannelProcessLocalPtr process(new ChannelProcessLocal(
        channelLocal,
        channelProcessRequester,
        pvRecord));
    channelLocal->addChannelProcess(process);
    channelProcessRequester->channelProcessConnect(Status::Ok, process);
    return process;
}


void ChannelProcessLocal::destroy()
{
    if(isDestroyed) return;
    isDestroyed = true;
    channelLocal->removeChannelProcess(getPtrSelf());
    channelLocal.reset();
    channelProcessRequester.reset();
    pvRecord.reset();
}

void ChannelProcessLocal::process(bool lastRequest)
{
    pvRecord->lock();
    pvRecord->process();
    pvRecord->unlock();
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelProcessRequester->processDone(status);
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
    virtual ~ChannelGetLocal(){destroy();}
    static ChannelGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelGetRequester::shared_pointer const & channelGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
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
      pvRecord(pvRecord),
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
    Mutex mutex;
    Lock thelock;
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
        pvRecord));
    channelLocal->addChannelGet(get);
    channelGetRequester->channelGetConnect(Status::Ok, get, pvStructure,bitSet);
    return get;
}


void ChannelGetLocal::destroy()
{
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
    } 
    bitSet->clear();
    pvRecord->lock();
    if(callProcess) pvRecord->process();
    pvCopy->updateCopySetBitSet(pvStructure, bitSet, false);
    pvRecord->unlock();
    if(firstTime) {
        bitSet->clear();
        bitSet->set(0);
        firstTime = false;
    } 
    channelGetRequester->getDone(Status::Ok);
    if(lastRequest) destroy();
}

class ChannelPutLocal :
    public ChannelPut,
    public std::tr1::enable_shared_from_this<ChannelPutLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutLocal);
    virtual ~ChannelPutLocal(){destroy();}
    static ChannelPutLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutRequester::shared_pointer const & channelPutRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
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
        PVRecordPtr const &pvRecord)
    :
      isDestroyed(false),
      callProcess(callProcess),
      channelLocal(channelLocal),
      channelPutRequester(channelPutRequester),
      pvCopy(pvCopy),
      pvStructure(pvStructure),
      bitSet(bitSet),
      pvRecord(pvRecord),
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
    Mutex mutex;
    Lock thelock;
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
        pvRecord));
    channelLocal->addChannelPut(put);
    channelPutRequester->channelPutConnect(Status::Ok, put, pvStructure,bitSet);
    return put;
}

void ChannelPutLocal::destroy()
{
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
    }
    bitSet->clear();
    bitSet->set(0);
    pvRecord->lock();
    pvCopy->updateCopyFromBitSet(pvStructure, bitSet, false);
    pvRecord->unlock();
    channelPutRequester->getDone(Status::Ok);
}

void ChannelPutLocal::put(bool lastRequest)
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutRequester->getDone(status);
    }
    pvRecord->lock();
    pvCopy->updateRecord(pvStructure, bitSet, false);
    if(callProcess) pvRecord->process();
    pvRecord->unlock();
    channelPutRequester->getDone(Status::Ok);
    if(lastRequest) destroy();
}


class ChannelPutGetLocal :
    public ChannelPutGet,
    public std::tr1::enable_shared_from_this<ChannelPutGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelPutGetLocal);
    virtual ~ChannelPutGetLocal(){destroy();}
    static ChannelPutGetLocalPtr create(
        ChannelLocalPtr const &channelLocal,
        ChannelPutGetRequester::shared_pointer const & channelPutGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
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
      pvRecord(pvRecord),
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
    Mutex mutex;
    Lock thelock;
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
    channelLocal->addChannelPutGet(putGet);
    channelPutGetRequester->channelPutGetConnect(
        Status::Ok, putGet, pvPutStructure,pvGetStructure);
    return putGet;
}


void ChannelPutGetLocal::destroy()
{
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
    } 
    putBitSet->clear();
    putBitSet->set(0);
    pvRecord->lock();
    pvPutCopy->updateRecord(pvPutStructure, putBitSet, false);
    if(callProcess) pvRecord->process();
    pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet, false);
    pvRecord->unlock();
    getBitSet->clear();
    getBitSet->set(0);
    channelPutGetRequester->putGetDone(Status::Ok);
    if(lastRequest) destroy();
}

void ChannelPutGetLocal::getPut()
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutGetRequester->getPutDone(status);
    } 
    pvRecord->lock();
    pvPutCopy->updateCopySetBitSet(pvPutStructure, putBitSet, false);
    pvRecord->unlock();
    putBitSet->clear();
    putBitSet->set(0);
    channelPutGetRequester->getPutDone(Status::Ok);
}

void ChannelPutGetLocal::getGet()
{
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelPutGetRequester->getGetDone(status);
    } 
    pvRecord->lock();
    pvGetCopy->updateCopySetBitSet(pvGetStructure, getBitSet, false);
    pvRecord->unlock();
    getBitSet->clear();
    getBitSet->set(0);
    channelPutGetRequester->getGetDone(Status::Ok);
}

class ChannelMonitorLocal :
    public Monitor
{
public:
    POINTER_DEFINITIONS(ChannelMonitorLocal);
    virtual ~ChannelMonitorLocal();
    static Monitor::shared_pointer create(
        ChannelProviderLocalPtr const &channelProvider,
        MonitorRequester::shared_pointer const & channelMonitorRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual Status start();
    virtual Status stop();
    virtual MonitorElementPtr poll();
    virtual void release(MonitorElementPtr const & monitorElement);
};

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

class ChannelArrayLocal :
    public ChannelArray
{
public:
    POINTER_DEFINITIONS(ChannelArrayLocal);
    virtual ~ChannelArrayLocal();
    static ChannelArray::shared_pointer create(
        ChannelProviderLocalPtr const &channelProvider,
        ChannelArray::shared_pointer const & channelArrayRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void putArray(bool lastRequest, int offset, int count);
    virtual void getArray(bool lastRequest, int offset, int count);
    virtual void setLength(bool lastRequest, int length, int capacity);
};

int ChannelLocalDebugLevel = 0;
void ChannelLocalDebug::setLevel(int level) {ChannelLocalDebugLevel = level;}
int ChannelLocalDebug::getLevel() {return ChannelLocalDebugLevel;}



ChannelLocal::ChannelLocal(
    ChannelProviderLocalPtr const & provider,
    ChannelRequester::shared_pointer const & requester,
    PVRecordPtr const & pvRecord)
:   provider(provider),
    requester(requester),
    pvRecord(pvRecord),
    beingDestroyed(false)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::ChannelLocal\n");
    }
}

ChannelLocal::~ChannelLocal()
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::~ChannelLocal\n");
    }
}

void ChannelLocal::destroy()
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::destroy beingDestroyed %s\n",
         (beingDestroyed ? "true" : "false"));
    }
    {
        Lock xx(mutex);
        if(beingDestroyed) return;
        beingDestroyed = true;
    }
    while(true) {
        std::set<ChannelProcess::shared_pointer>::iterator it;
        it = channelProcessList.begin();
        if(it==channelProcessList.end()) break;
        it->get()->destroy();
        channelProcessList.erase(it);
    }
    while(true) {
        std::set<ChannelGet::shared_pointer>::iterator it;
        it = channelGetList.begin();
        if(it==channelGetList.end()) break;
        it->get()->destroy();
        channelGetList.erase(it);
    }
    while(true) {
        std::set<ChannelPut::shared_pointer>::iterator it;
        it = channelPutList.begin();
        if(it==channelPutList.end()) break;
        it->get()->destroy();
        channelPutList.erase(it);
    }
    while(true) {
        std::set<ChannelPutGet::shared_pointer>::iterator it;
        it = channelPutGetList.begin();
        if(it==channelPutGetList.end()) break;
        it->get()->destroy();
        channelPutGetList.erase(it);
    }
    while(true) {
        std::set<Monitor::shared_pointer>::iterator it;
        it = channelMonitorList.begin();
        if(it==channelMonitorList.end()) break;
        it->get()->destroy();
        channelMonitorList.erase(it);
    }
    while(true) {
        std::set<ChannelRPC::shared_pointer>::iterator it;
        it = channelRPCList.begin();
        if(it==channelRPCList.end()) break;
        it->get()->destroy();
        channelRPCList.erase(it);
    }
    while(true) {
        std::set<ChannelArray::shared_pointer>::iterator it;
        it = channelArrayList.begin();
        if(it==channelArrayList.end()) break;
        it->get()->destroy();
        channelArrayList.erase(it);
    }
    provider->removeChannel(getPtrSelf());
}


void ChannelLocal::addChannelProcess(ChannelProcess::shared_pointer const & channelProcess)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelProcess\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelProcessList.insert(channelProcess);
}

void ChannelLocal::addChannelGet(ChannelGet::shared_pointer const &channelGet)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelGet\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelGetList.insert(channelGet);
}

void ChannelLocal::addChannelPut(ChannelPut::shared_pointer const &channelPut)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelPut\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelPutList.insert(channelPut);
}

void ChannelLocal::addChannelPutGet(ChannelPutGet::shared_pointer const &channelPutGet)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelPutGet\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelPutGetList.insert(channelPutGet);
}

void ChannelLocal::addChannelMonitor(Monitor::shared_pointer const &monitor)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelMonitor\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelMonitorList.insert(monitor);
}

void ChannelLocal::addChannelRPC(ChannelRPC::shared_pointer const &channelRPC)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelRPC\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelRPCList.insert(channelRPC);
}

void ChannelLocal::addChannelArray(ChannelArray::shared_pointer const &channelArray)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::addChannelArray\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelArrayList.insert(channelArray);
}

void ChannelLocal::removeChannelProcess(ChannelProcess::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelProcess\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelProcessList.erase(ref);
}

void ChannelLocal::removeChannelGet(ChannelGet::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelGet\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelGetList.erase(ref);
}

void ChannelLocal::removeChannelPut(ChannelPut::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelPut\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelPutList.erase(ref);
}

void ChannelLocal::removeChannelPutGet(ChannelPutGet::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelPutGet\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelPutGetList.erase(ref);
}

void ChannelLocal::removeChannelMonitor(Monitor::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelMonitor\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelMonitorList.erase(ref);
}

void ChannelLocal::removeChannelRPC(ChannelRPC::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelRPC\n");
    }
    Lock xx(mutex);
    if(beingDestroyed) return;
    channelRPCList.erase(ref);
}

void ChannelLocal::removeChannelArray(ChannelArray::shared_pointer const &ref)
{
    if(ChannelLocalDebug::getLevel()>0) {
         printf("ChannelLocal::removeChannelArray\n");
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

ChannelProvider::shared_pointer ChannelLocal::getProvider()
{
    return provider;
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
        StructureConstPtr structure = pvRecord->getPVRecordStructure()->getPVStructure()->getStructure();
        requester->getDone(Status::Ok,structure);
        return;
    } 
    PVFieldPtr pvField =  pvRecord->getPVRecordStructure()->getPVStructure()->getSubField(subField);
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
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelMonitor not supported"));
    Monitor::shared_pointer thisPointer = dynamic_pointer_cast<Monitor>(getPtrSelf());
    monitorRequester->monitorConnect(
        status,
        thisPointer,
        StructureConstPtr());
    return Monitor::shared_pointer();
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
    printf("ChannelLocal provides access to service\n");
}

void ChannelLocal::printInfo(StringBuilder out)
{
    *out += "ChannelLocal provides access to service";
}

}}
