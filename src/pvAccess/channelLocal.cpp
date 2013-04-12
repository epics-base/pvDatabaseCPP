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

/*
class ChannelRequestLocal :
    public ChannelRequest
{
public:
    POINTER_DEFINITIONS(ChannelRequestLocal)
    ChannelRequestLocal() :thelock(mutex) {}
    virtual ~ChannelRequestLocal() {}
    virtual void destroy(){}
    virtual void lock() {thelock.lock();}
    virtual void unlock() {thelock.unlock();}
private:
    Mutex mutex;
    Lock thelock;
};
*/
    

class ChannelProcessLocal :
    public ChannelProcess
{
public:
    POINTER_DEFINITIONS(ChannelProcessLocal);
    virtual ~ChannelProcessLocal();
    static ChannelProcess::shared_pointer create(
        ChannelProviderLocalPtr const &channelProvider,
        ChannelProcess::shared_pointer const & channelProcessRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void process(bool lastRequest);
};

class ChannelGetLocal :
    public ChannelGet,
    public std::tr1::enable_shared_from_this<ChannelGetLocal>
{
public:
    POINTER_DEFINITIONS(ChannelGetLocal);
    virtual ~ChannelGetLocal(){}
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
    if(callProcess) pvRecord->process();
    if(isDestroyed) {
         Status status(
             Status::Status::STATUSTYPE_ERROR,
            "was destroyed");
         channelGetRequester->getDone(status);
    } 
    pvCopy->updateCopySetBitSet(pvStructure, bitSet, false);
    if(firstTime) {
        bitSet->clear();
        bitSet->set(0);
        firstTime = false;
    } 
    channelGetRequester->getDone(Status::Ok);
    if(lastRequest) destroy();
}


class ChannelLocalPut :
    public ChannelPut
{
public:
    POINTER_DEFINITIONS(ChannelLocalPut);
    virtual ~ChannelLocalPut();
    static ChannelPut::shared_pointer create(
        ChannelProviderLocalPtr const &channelProvider,
        ChannelPut::shared_pointer const & channelPutRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void put(bool lastRequest);
    virtual void get(bool lastRequest);
};

class ChannelPutGetLocal :
    public ChannelPutGet
{
public:
    POINTER_DEFINITIONS(ChannelPutGetLocal);
    virtual ~ChannelPutGetLocal();
    static ChannelPutGet::shared_pointer create(
        ChannelProviderLocalPtr const &channelProvider,
        ChannelPutGet::shared_pointer const & channelPutGetRequester,
        PVStructurePtr const & pvRequest,
        PVRecordPtr const &pvRecord);
    virtual void putGet(bool lastRequest);
    virtual void getPut();
    virtual void getGet();
};

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
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelProcess not supported"));
    channelProcessRequester->channelProcessConnect(
        status,
        ChannelProcess::shared_pointer());
    return ChannelProcess::shared_pointer();
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
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelPut not supported"));
    channelPutRequester->channelPutConnect(
        status,
        ChannelPut::shared_pointer(),
        PVStructure::shared_pointer(),
        BitSet::shared_pointer());
    return ChannelPut::shared_pointer();
}

ChannelPutGet::shared_pointer ChannelLocal::createChannelPutGet(
        ChannelPutGetRequester::shared_pointer const &channelPutGetRequester,
        PVStructure::shared_pointer const &pvRequest)
{
    Status status(Status::STATUSTYPE_ERROR,
        String("ChannelPutGet not supported"));
    channelPutGetRequester->channelPutGetConnect(
        status,
        ChannelPutGet::shared_pointer(),
        PVStructure::shared_pointer(),
        PVStructure::shared_pointer());
    return ChannelPutGet::shared_pointer();
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
