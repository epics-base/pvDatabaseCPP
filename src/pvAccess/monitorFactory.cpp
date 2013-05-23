/* monitorFactory.cpp */
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

#include <pv/channelProviderLocal.h>
#include <pv/channelProviderLocal.h>
#include <pv/bitSetUtil.h>
#include <pv/queue.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;

static MonitorAlgorithmCreatePtr nullMonitorAlgorithmCreate;
static MonitorPtr nullMonitor;
static MonitorElementPtr NULLMonitorElement;

static ConvertPtr convert = getConvert();

//class MonitorFieldNode;
//typedef std::tr1::shared_ptr<MonitorFieldNode> MonitorFieldNodePtr;

class MonitorQueue;
typedef std::tr1::shared_ptr<MonitorQueue> MonitorQueuePtr;
class NOQueue;
typedef std::tr1::shared_ptr<NOQueue> NOQueuePtr;
class RealQueue;
typedef std::tr1::shared_ptr<RealQueue> RealQueuePtr;

//class MonitorFieldNode
//{
//public:
//    MonitorAlgorithmPtr monitorAlgorithm;
//    size_t bitOffset; // pv pvCopy
//};

class MonitorQueue :
    public std::tr1::enable_shared_from_this<MonitorQueue>
{
public:
    POINTER_DEFINITIONS(MonitorQueue);
    virtual ~MonitorQueue(){}
    virtual Status start() = 0;
    virtual void stop() = 0;
    virtual bool dataChanged() = 0;
    virtual MonitorElementPtr poll() = 0;
    virtual void release(MonitorElementPtr const &monitorElement) = 0;
protected:
    MonitorQueuePtr getPtrSelf()
    {
        return shared_from_this();
    }
};

class NOQueue :
    public MonitorQueue
{
public:
    POINTER_DEFINITIONS(NOQueue);
    virtual ~NOQueue(){}
    NOQueue(
        MonitorLocalPtr const &monitorLocal);
   
    virtual Status start();
    virtual void stop();
    virtual bool dataChanged();
    virtual MonitorElementPtr poll();
    virtual void release(MonitorElementPtr const &monitorElement);
private:
    MonitorLocalPtr monitorLocal;
    PVStructurePtr pvCopyStructure;
    MonitorElementPtr monitorElement;
    bool gotMonitor;
    bool wasReleased;
    BitSetPtr changedBitSet;
    BitSetPtr overrunBitSet;
    Mutex mutex;
};

class RealQueue :
    public MonitorQueue
{
public:
    POINTER_DEFINITIONS(RealQueue);
    virtual ~RealQueue(){}
    RealQueue(
        MonitorLocalPtr const &monitorLocal,
        std::vector<MonitorElementPtr> &monitorElementArray);
    virtual Status start();
    virtual void stop();
    virtual bool dataChanged();
    virtual MonitorElementPtr poll();
    virtual void release(MonitorElementPtr const &monitorElement);
private:
    MonitorLocalPtr monitorLocal;
    Queue<MonitorElement> queue;
    bool queueIsFull;
    MonitorElementPtr monitorElement;
    Mutex mutex;
};

    
class MonitorLocal :
    public Monitor,
    public PVCopyMonitorRequester,
    public std::tr1::enable_shared_from_this<MonitorLocal>
{
public:
    POINTER_DEFINITIONS(MonitorLocal);
    virtual ~MonitorLocal();
    virtual Status start();
    virtual Status stop();
    virtual MonitorElementPtr poll();
    virtual void destroy();
    virtual void dataChanged();
    virtual void unlisten();
    virtual void release(MonitorElementPtr const & monitorElement);
    bool init(PVStructurePtr const & pvRequest);
    MonitorLocal(
        MonitorRequester::shared_pointer const & channelMonitorRequester,
        PVRecordPtr const &pvRecord,
        ChannelLocalDebugPtr const &channelLocalDebug);
    PVCopyPtr getPVCopy() { return pvCopy;}
    PVCopyMonitorPtr getPVCopyMonitor() { return pvCopyMonitor;}
private:
    MonitorLocalPtr getPtrSelf()
    {
        return shared_from_this();
    }
    MonitorRequester::shared_pointer monitorRequester;
    PVRecordPtr pvRecord;
    ChannelLocalDebugPtr channelLocalDebug;
    bool isDestroyed;
    bool firstMonitor;
    PVCopyPtr pvCopy;
    MonitorQueuePtr queue;
    PVCopyMonitorPtr pvCopyMonitor;
//    std::list<MonitorFieldNodePtr> monitorFieldList;
    Mutex mutex;
};

MonitorLocal::MonitorLocal(
    MonitorRequester::shared_pointer const & channelMonitorRequester,
    PVRecordPtr const &pvRecord,
    ChannelLocalDebugPtr const &channelLocalDebug)
: monitorRequester(channelMonitorRequester),
  pvRecord(pvRecord),
  channelLocalDebug(channelLocalDebug),
  isDestroyed(false),
  firstMonitor(true)
{
}

MonitorLocal::~MonitorLocal()
{
std::cout << "MonitorLocal::~MonitorLocal()" << std::endl;
}

void MonitorLocal::destroy()
{
std::cout << "MonitorLocal::destroy " << isDestroyed << std::endl;
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    unlisten();
    stop();
//    monitorFieldList.clear();
    pvCopyMonitor.reset();
    queue.reset();
    pvCopy.reset();
    monitorRequester.reset();
    pvRecord.reset();
}

Status MonitorLocal::start()
{
//std::cout << "MonitorLocal::start" << std::endl;
    firstMonitor = true;
    return queue->start();
}

Status MonitorLocal::stop()
{
std::cout << "MonitorLocal::stop" << std::endl;
    pvCopyMonitor->stopMonitoring();
    queue->stop();
    return Status::Ok;
}

MonitorElementPtr MonitorLocal::poll()
{
//std::cout << "MonitorLocal::poll" << std::endl;
    return queue->poll();
}

void MonitorLocal::dataChanged()
{
//std::cout << "MonitorLocal::dataChanged" << std::endl;
    if(firstMonitor) {
    	queue->dataChanged();
    	firstMonitor = false;
    	monitorRequester->monitorEvent(getPtrSelf());
    	return;
    }
    if(queue->dataChanged()) {
    	monitorRequester->monitorEvent(getPtrSelf());
    }
}

void MonitorLocal::unlisten()
{
std::cout << "MonitorLocal::unlisten" << std::endl;
    monitorRequester->unlisten(getPtrSelf());
}

void MonitorLocal::release(MonitorElementPtr const & monitorElement)
{
//std::cout << "MonitorLocal::release" << std::endl;
    queue->release(monitorElement);
}

bool MonitorLocal::init(PVStructurePtr const & pvRequest)
{
    PVFieldPtr pvField;
    PVStructurePtr pvOptions;
    size_t queueSize = 2;
    pvField = pvRequest->getSubField("record._options");
    if(pvField.get()!=NULL) {
        pvOptions = static_pointer_cast<PVStructure>(pvField);
        pvField = pvOptions->getSubField("queueSize");
        if(pvField.get()!=NULL) {
            PVStringPtr pvString = pvOptions->getStringField("queueSize");
            if(pvString.get()!=NULL) {
                int32 size;
                std::stringstream ss;
                ss << pvString->get();
                ss >> size;
                queueSize = size;
            }
        }
    }
    if(queueSize==1) {
        monitorRequester->message("queueSize can not be 1",errorMessage);
        return false;
    }
    pvField = pvRequest->getSubField("field");
    if(pvField.get()==NULL) {
        pvCopy = PVCopy::create(pvRecord,pvRequest,"");
        if(pvCopy.get()==NULL) {
            monitorRequester->message("illegal pvRequest",errorMessage);
            return false;
        }
    } else {
        if(pvField->getField()->getType()!=structure) {
            monitorRequester->message("illegal pvRequest",errorMessage);
            return false;
        }
        pvCopy = PVCopy::create(pvRecord,pvRequest,"field");
        if(pvCopy.get()==NULL) {
            monitorRequester->message("illegal pvRequest",errorMessage);
            return false;
        }
    }
    pvCopyMonitor = pvCopy->createPVCopyMonitor(getPtrSelf());
    // MARTY MUST IMPLEMENT periodic
    if(queueSize==0) {
        queue = NOQueuePtr(new NOQueue(getPtrSelf()));
    } else {
        std::vector<MonitorElementPtr> monitorElementArray;
        monitorElementArray.reserve(queueSize);
        for(size_t i=0; i<queueSize; i++) {
             PVStructurePtr pvStructure = pvCopy->createPVStructure();
             MonitorElementPtr monitorElement(
                 new MonitorElement(pvStructure));
             monitorElementArray.push_back(monitorElement);
        }
        queue = RealQueuePtr(new RealQueue(getPtrSelf(),monitorElementArray));
    }
    // MARTY MUST IMPLEMENT algorithm
    monitorRequester->monitorConnect(
        Status::Ok,
        getPtrSelf(),
        pvCopy->getStructure());
    return true;
}


MonitorFactory::MonitorFactory()
: isDestroyed(false)
{
}

MonitorFactory::~MonitorFactory()
{
}

void MonitorFactory::destroy()
{
std::cout << "MonitorFactory::destroy " << isDestroyed << std::endl;
    Lock lock(mutex);
    if(isDestroyed) return;
    isDestroyed = true;
    while(true) {
        std::set<MonitorLocalPtr>::iterator it;
        it = monitorLocalList.begin();
        if(it==monitorLocalList.end()) break;
        monitorLocalList.erase(it);
        lock.unlock();
            it->get()->destroy();
        lock.lock();
    }
}

MonitorPtr MonitorFactory::createMonitor(
    PVRecordPtr const & pvRecord,
    MonitorRequester::shared_pointer const & monitorRequester,
    PVStructurePtr const & pvRequest,
    ChannelLocalDebugPtr const &channelLocalDebug)
{
    Lock xx(mutex);
    if(isDestroyed) {
        monitorRequester->message("MonitorFactory is destroyed",errorMessage);
        return nullMonitor;
    }
    MonitorLocalPtr monitor(new MonitorLocal(
        monitorRequester,pvRecord,channelLocalDebug));
    bool result = monitor->init(pvRequest);
    if(!result) return nullMonitor;
    if(channelLocalDebug->getLevel()>0)
    {
        std::cout << "MonitorFactory::createMonitor";
        std::cout << " recordName " << pvRecord->getRecordName() << std::endl;
    }
    monitorLocalList.insert(monitor);
    return monitor;
}

void MonitorFactory::registerMonitorAlgorithmCreate(
        MonitorAlgorithmCreatePtr const &monitorAlgorithmCreate)
{
    Lock xx(mutex);
    if(isDestroyed) return;
//    monitorAlgorithmCreateList.insert(monitorAlgorithmCreate);
}

MonitorAlgorithmCreatePtr MonitorFactory::getMonitorAlgorithmCreate(
    String algorithmName)
{
    Lock xx(mutex);
    if(isDestroyed) return nullMonitorAlgorithmCreate;
//    std::set<MonitorAlgorithmCreatePtr>::iterator iter;
//    for(iter = monitorAlgorithmCreateList.begin();
//    iter!= monitorAlgorithmCreateList.end();
//    ++iter)
//    {
//        if((*iter)->getAlgorithmName().compare(algorithmName))
//             return *iter;
//    }
    return nullMonitorAlgorithmCreate;
}

NOQueue::NOQueue(
    MonitorLocalPtr const &monitorLocal)
:  monitorLocal(monitorLocal),
   pvCopyStructure(monitorLocal->getPVCopy()->createPVStructure()),
   monitorElement(new MonitorElement(pvCopyStructure)),
   gotMonitor(false),
   wasReleased(false),
   changedBitSet(new BitSet(pvCopyStructure->getNumberFields())),
   overrunBitSet(new BitSet(pvCopyStructure->getNumberFields()))
{
    monitorElement->pvStructurePtr = pvCopyStructure;
    monitorElement->changedBitSet =
        BitSetPtr(new BitSet(pvCopyStructure->getNumberFields()));
    monitorElement->overrunBitSet =
        BitSetPtr(new BitSet(pvCopyStructure->getNumberFields()));
}

Status NOQueue::start()
{
    Lock xx(mutex);
    gotMonitor = true;
    wasReleased = true;
    changedBitSet->clear();
    overrunBitSet->clear();
    monitorLocal->getPVCopyMonitor()->startMonitoring(
        monitorElement->changedBitSet,
        monitorElement->overrunBitSet);
    return Status::Ok;
}

void NOQueue::stop()
{
}

bool NOQueue::dataChanged()
{
   Lock xx(mutex);
   (*changedBitSet) |= (*monitorElement->changedBitSet);
   (*overrunBitSet) |= (*monitorElement->overrunBitSet);
   gotMonitor = true;
   return wasReleased ? true : false;
}

MonitorElementPtr NOQueue::poll()
{
   Lock xx(mutex);
   if(!gotMonitor) return NULLMonitorElement;
   BitSetPtr changed = monitorElement->changedBitSet;
   BitSetPtr overrun = monitorElement->overrunBitSet;
   (*changed) |= (*changedBitSet);
   (*overrun) |= (*overrunBitSet);
   monitorLocal->getPVCopy()->updateCopyFromBitSet(
       pvCopyStructure, changed,true);
   BitSetUtil::compress(changed,pvCopyStructure);
   BitSetUtil::compress(overrun,pvCopyStructure);
   changedBitSet->clear();
   overrunBitSet->clear();
   return monitorElement;
}

void NOQueue::release(MonitorElementPtr const &monitorElement)
{
   Lock xx(mutex);
   gotMonitor = false;
   monitorElement->changedBitSet->clear();
   monitorElement->overrunBitSet->clear();
}

RealQueue::RealQueue(
    MonitorLocalPtr const &monitorLocal,
    std::vector<MonitorElementPtr> &monitorElementArray)
:  monitorLocal(monitorLocal),
   queue(monitorElementArray),
   queueIsFull(false)
{
}

Status RealQueue::start()
{
    Lock xx(mutex);
    queue.clear();
    monitorElement = queue.getFree();
    monitorElement->changedBitSet->clear();
    monitorElement->overrunBitSet->clear();
    monitorLocal->getPVCopyMonitor()->startMonitoring(
        monitorElement->changedBitSet,
        monitorElement->overrunBitSet);
    return Status::Ok;
}

void RealQueue::stop()
{
}

bool RealQueue::dataChanged()
{
    Lock xx(mutex);
    PVStructurePtr pvStructure = monitorElement->pvStructurePtr;
    monitorLocal->getPVCopy()->updateCopyFromBitSet(
        pvStructure,monitorElement->changedBitSet,false);
    MonitorElementPtr newElement = queue.getFree();
    if(newElement==NULL) {
        queueIsFull = true;
        return true;
    }
    BitSetUtil::compress(monitorElement->changedBitSet,pvStructure);
    BitSetUtil::compress(monitorElement->overrunBitSet,pvStructure);
    convert->copy(pvStructure,newElement->pvStructurePtr);
    newElement->changedBitSet->clear();
    newElement->overrunBitSet->clear();
    monitorLocal->getPVCopyMonitor()->switchBitSets(
        newElement->changedBitSet,newElement->overrunBitSet,false);
    queue.setUsed(monitorElement);
    monitorElement = newElement;
    return true;
}

MonitorElementPtr RealQueue::poll()
{
   Lock xx(mutex);
   return queue.getUsed();
}

void RealQueue::release(MonitorElementPtr const &currentElement)
{
   Lock xx(mutex);
   queue.releaseUsed(currentElement);
   currentElement->changedBitSet->clear();
   currentElement->overrunBitSet->clear();
   if(!queueIsFull) return;
   queueIsFull = false;
    PVStructurePtr pvStructure = monitorElement->pvStructurePtr;
    MonitorElementPtr newElement = queue.getFree();
    BitSetUtil::compress(monitorElement->changedBitSet,pvStructure);
    BitSetUtil::compress(monitorElement->overrunBitSet,pvStructure);
    convert->copy(pvStructure,newElement->pvStructurePtr);
    newElement->changedBitSet->clear();
    newElement->overrunBitSet->clear();
    monitorLocal->getPVCopyMonitor()->switchBitSets(
        newElement->changedBitSet,newElement->overrunBitSet,false);
    queue.setUsed(monitorElement);
    monitorElement = newElement;
   
}

MonitorFactoryPtr getMonitorFactory()
{
    static MonitorFactoryPtr monitorFactoryPtr;
    static Mutex mutex;
    Lock xx(mutex);

    if(monitorFactoryPtr.get()==NULL) {
        monitorFactoryPtr = MonitorFactoryPtr(
            new MonitorFactory());
    }
    return monitorFactoryPtr;
}

}}
