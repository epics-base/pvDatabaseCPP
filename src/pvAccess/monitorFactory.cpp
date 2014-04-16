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

#include <pv/thread.h>
#include <pv/bitSetUtil.h>
#include <pv/queue.h>
#include <pv/timeStamp.h>

#define epicsExportSharedSymbols

#include <pv/channelProviderLocal.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::cout;
using std::endl;

static MonitorAlgorithmCreatePtr nullMonitorAlgorithmCreate;
static MonitorPtr nullMonitor;
static MonitorElementPtr NULLMonitorElement;
static Status wasDestroyedStatus(Status::STATUSTYPE_ERROR,"was destroyed");

static ConvertPtr convert = getConvert();

//class MonitorFieldNode;
//typedef std::tr1::shared_ptr<MonitorFieldNode> MonitorFieldNodePtr;

class ElementQueue;
typedef std::tr1::shared_ptr<ElementQueue> ElementQueuePtr;
class MultipleElementQueue;
typedef std::tr1::shared_ptr<MultipleElementQueue> MultipleElementQueuePtr;

//class MonitorFieldNode
//{
//public:
//    MonitorAlgorithmPtr monitorAlgorithm;
//    size_t bitOffset; // pv pvCopy
//};

class ElementQueue :
    public Monitor,
    public std::tr1::enable_shared_from_this<ElementQueue>
{
public:
    POINTER_DEFINITIONS(ElementQueue);
    virtual ~ElementQueue(){}
    virtual bool dataChanged() = 0;
protected:
    ElementQueuePtr getPtrSelf()
    {
        return shared_from_this();
    }
};


typedef Queue<MonitorElement> MonitorElementQueue;
typedef std::tr1::shared_ptr<MonitorElementQueue> MonitorElementQueuePtr;

class MultipleElementQueue :
    public ElementQueue
{
public:
    POINTER_DEFINITIONS(MultipleElementQueue);
    virtual ~MultipleElementQueue(){}
    MultipleElementQueue(
        MonitorLocalPtr const &monitorLocal,
        MonitorElementQueuePtr const &queue,
        size_t nfields);
    virtual void destroy(){}
    virtual Status start();
    virtual Status stop();
    virtual bool dataChanged();
    virtual MonitorElementPtr poll();
    virtual void release(MonitorElementPtr const &monitorElement);
private:
    std::tr1::weak_ptr<MonitorLocal> monitorLocal;
    MonitorElementQueuePtr queue;
    BitSetPtr changedBitSet;
    BitSetPtr overrunBitSet;
    MonitorElementPtr latestMonitorElement;
    bool queueIsFull;
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
        PVRecordPtr const &pvRecord);
    PVCopyPtr getPVCopy() { return pvCopy;}
    PVCopyMonitorPtr getPVCopyMonitor() { return pvCopyMonitor;}
private:
    MonitorLocalPtr getPtrSelf()
    {
        return shared_from_this();
    }
    MonitorRequester::shared_pointer monitorRequester;
    PVRecordPtr pvRecord;
    bool isDestroyed;
    bool firstMonitor;
    PVCopyPtr pvCopy;
    ElementQueuePtr queue;
    PVCopyMonitorPtr pvCopyMonitor;
    Mutex mutex;
};

MonitorLocal::MonitorLocal(
    MonitorRequester::shared_pointer const & channelMonitorRequester,
    PVRecordPtr const &pvRecord)
: monitorRequester(channelMonitorRequester),
  pvRecord(pvRecord),
  isDestroyed(false),
  firstMonitor(true)
{
}

MonitorLocal::~MonitorLocal()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorLocal::~MonitorLocal()" << endl;
    }
}

void MonitorLocal::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorLocal::destroy " << isDestroyed << endl;
    }
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        isDestroyed = true;
    }
    unlisten();
    stop();
    pvCopyMonitor->destroy();
    pvCopy->destroy();
    pvCopyMonitor.reset();
    queue.reset();
    pvCopy.reset();
}

Status MonitorLocal::start()
{
    Lock xx(mutex);
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorLocal::start() "  << endl;
    }
    if(isDestroyed) return wasDestroyedStatus;
    firstMonitor = true;
    return queue->start();
}

Status MonitorLocal::stop()
{
    pvCopyMonitor->stopMonitoring();
    {
        Lock xx(mutex);
        if(pvRecord->getTraceLevel()>0){
            cout << "MonitorLocal::stop() "  << endl;
        }
        if(!isDestroyed) queue->stop();
    }
    return Status::Ok;
}

MonitorElementPtr MonitorLocal::poll()
{
    Lock xx(mutex);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::poll() "  << endl;
    }
    if(isDestroyed) {
        return NULLMonitorElement;
    }
    return queue->poll();
}

void MonitorLocal::release(MonitorElementPtr const & monitorElement)
{
    Lock xx(mutex);
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::release() "  << endl;
    }
    if(isDestroyed) {
        return;
    }
    queue->release(monitorElement);
}

void MonitorLocal::dataChanged()
{
    if(pvRecord->getTraceLevel()>1)
    {
        cout << "MonitorLocal::dataChanged() "  "firstMonitor " << firstMonitor << endl;
    }
    bool getMonitorEvent = false;
    {
        Lock xx(mutex);
        if(isDestroyed) return;
        getMonitorEvent = queue->dataChanged();
        firstMonitor = false;
    }
    if(getMonitorEvent) monitorRequester->monitorEvent(getPtrSelf());
}

void MonitorLocal::unlisten()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorLocal::unlisten() "  << endl;
    }
    monitorRequester->unlisten(getPtrSelf());
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
    if(queueSize<2) queueSize = 2;
    std::vector<MonitorElementPtr> monitorElementArray;
    monitorElementArray.reserve(queueSize);
    size_t nfields = 0;
    for(size_t i=0; i<queueSize; i++) {
         PVStructurePtr pvStructure = pvCopy->createPVStructure();
         if(nfields==0) nfields = pvStructure->getNumberFields();
         MonitorElementPtr monitorElement(
             new MonitorElement(pvStructure));
         monitorElementArray.push_back(monitorElement);
    }
    MonitorElementQueuePtr elementQueue(new MonitorElementQueue(monitorElementArray));
    queue = MultipleElementQueuePtr(new MultipleElementQueue(
        getPtrSelf(),
        elementQueue,
        nfields));
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
    Lock lock(mutex);
    if(isDestroyed) return;
    isDestroyed = true;
}

MonitorPtr MonitorFactory::createMonitor(
    PVRecordPtr const & pvRecord,
    MonitorRequester::shared_pointer const & monitorRequester,
    PVStructurePtr const & pvRequest)
{
    Lock xx(mutex);
    if(isDestroyed) {
        monitorRequester->message("MonitorFactory is destroyed",errorMessage);
        return nullMonitor;
    }
    MonitorLocalPtr monitor(new MonitorLocal(
        monitorRequester,pvRecord));
    bool result = monitor->init(pvRequest);
    if(!result) return nullMonitor;
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "MonitorFactory::createMonitor";
        cout << " recordName " << pvRecord->getRecordName() << endl;
    }
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
//    std::multiset<MonitorAlgorithmCreatePtr>::iterator iter;
//    for(iter = monitorAlgorithmCreateList.begin();
//    iter!= monitorAlgorithmCreateList.end();
//    ++iter)
//    {
//        if((*iter)->getAlgorithmName().compare(algorithmName))
//             return *iter;
//    }
    return nullMonitorAlgorithmCreate;
}

MultipleElementQueue::MultipleElementQueue(
    MonitorLocalPtr const &monitorLocal,
    MonitorElementQueuePtr const &queue,
    size_t nfields)
:  monitorLocal(monitorLocal),
   queue(queue),
   changedBitSet(new BitSet(nfields)),
   overrunBitSet(new BitSet(nfields)),
   queueIsFull(false)
{
}

Status MultipleElementQueue::start()
{
    queue->clear();
    queueIsFull = false;
    changedBitSet->clear();
    overrunBitSet->clear();
    MonitorLocalPtr ml = monitorLocal.lock();
    if(ml==NULL) return wasDestroyedStatus;
    ml->getPVCopyMonitor()->startMonitoring(changedBitSet,overrunBitSet);
    return Status::Ok;
}

Status MultipleElementQueue::stop()
{
    return Status::Ok;
}

bool MultipleElementQueue::dataChanged()
{
    MonitorLocalPtr ml = monitorLocal.lock();
    if(ml==NULL) return false;
    if(queueIsFull) {
        MonitorElementPtr monitorElement = latestMonitorElement;
        PVStructurePtr pvStructure = monitorElement->pvStructurePtr;
        ml->getPVCopy()->updateCopyFromBitSet(pvStructure,changedBitSet);
        (*monitorElement->changedBitSet)|= (*changedBitSet);
        (*monitorElement->overrunBitSet)|= (*changedBitSet);
        changedBitSet->clear();
        overrunBitSet->clear();
        return false;
    }
    MonitorElementPtr monitorElement = queue->getFree();
    if(monitorElement==NULL) {
        throw  std::logic_error(String("MultipleElementQueue::dataChanged() logic error"));
    }
    if(queue->getNumberFree()==0){
        queueIsFull = true;
        latestMonitorElement = monitorElement;
    }
    PVStructurePtr pvStructure = monitorElement->pvStructurePtr;
    ml->getPVCopy()->updateCopyFromBitSet(
        pvStructure,changedBitSet);
    BitSetUtil::compress(changedBitSet,pvStructure);
    BitSetUtil::compress(overrunBitSet,pvStructure);
    monitorElement->changedBitSet->clear();
    (*monitorElement->changedBitSet)|=(*changedBitSet);
    monitorElement->overrunBitSet->clear();
    (*monitorElement->overrunBitSet)|=(*overrunBitSet);
    changedBitSet->clear();
    overrunBitSet->clear();
    queue->setUsed(monitorElement);
    return true;
}

MonitorElementPtr MultipleElementQueue::poll()
{
    return queue->getUsed();
}

void MultipleElementQueue::release(MonitorElementPtr const &currentElement)
{
    if(queueIsFull) {
        MonitorElementPtr monitorElement = latestMonitorElement;
        PVStructurePtr pvStructure = monitorElement->pvStructurePtr;
        BitSetUtil::compress(monitorElement->changedBitSet,pvStructure);
        BitSetUtil::compress(monitorElement->overrunBitSet,pvStructure);
        queueIsFull = false;
        latestMonitorElement.reset();
    }
    queue->releaseUsed(currentElement);
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
