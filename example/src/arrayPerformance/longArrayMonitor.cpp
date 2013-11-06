/* longArrayMonitor.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.08.09
 */

#include <epicsThread.h>
#include <pv/longArrayMonitor.h>
#include <pv/caProvider.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;

static String requesterName("longArrayMonitor");

static void messagePvt(String const & message, MessageType messageType)
{
    cout << requesterName << " message " << message << endl;
}

class LAMChannelRequester :
    public ChannelRequester
{
public:
    LAMChannelRequester(LongArrayMonitorPtr const &longArrayMonitor)
    : longArrayMonitor(longArrayMonitor)
    {}
    virtual ~LAMChannelRequester(){}
    virtual void destroy(){longArrayMonitor.reset();}
    virtual String getRequesterName() { return requesterName;}
    virtual void message(String const & message, MessageType messageType)
       { messagePvt(message,messageType);}
    virtual void channelCreated(const Status& status, Channel::shared_pointer const & channel);
    virtual void channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState);
private:
    LongArrayMonitorPtr longArrayMonitor;
};

void LAMChannelRequester::channelCreated(const Status& status, Channel::shared_pointer const & channel)
{
    if(!status.isOK()) messagePvt(status.getMessage(),errorMessage);
    longArrayMonitor->status = status;
    longArrayMonitor->channel = channel;
    longArrayMonitor->event.signal();
}

void LAMChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState)
{
   MessageType messageType = (connectionState==Channel::CONNECTED ? infoMessage : errorMessage);
   messagePvt("channelStateChange",messageType);
}


class LAMMonitorRequester :
    public MonitorRequester,
    public epicsThreadRunable
{
public:
    LAMMonitorRequester(LongArrayMonitorPtr const &longArrayMonitor)
    : longArrayMonitor(longArrayMonitor),
      isDestroyed(false),
      runReturned(false),
      threadName("longArrayMonitor")
    {}
    virtual ~LAMMonitorRequester(){}
    void init();
    virtual void destroy();
    virtual void run();
    virtual String getRequesterName() { return requesterName;}
    virtual void message(String const & message, MessageType messageType)
       { messagePvt(message,messageType);}
    virtual void monitorConnect(Status const & status,
        MonitorPtr const & monitor, StructureConstPtr const & structure);
    virtual void monitorEvent(MonitorPtr const & monitor);
    virtual void unlisten(MonitorPtr const & monitor);
private:
    void handleMonitor();
    LongArrayMonitorPtr longArrayMonitor;
    bool isDestroyed;
    bool runReturned;
    epics::pvData::String threadName;
    Event event;
    Mutex mutex;
    std::auto_ptr<epicsThread> thread;
};

void LAMMonitorRequester::init()
{
     thread = std::auto_ptr<epicsThread>(new epicsThread(
        *this,
        threadName.c_str(),
        epicsThreadGetStackSize(epicsThreadStackSmall),
        epicsThreadPriorityLow));
     thread->start();
}

void LAMMonitorRequester::destroy()
{
    if(isDestroyed) return;
    isDestroyed = true;
    event.signal();
    while(true) {
        if(runReturned) break;
        epicsThreadSleep(.01);
    }
    thread->exitWait();
    longArrayMonitor.reset();
}


void LAMMonitorRequester::monitorConnect(Status const & status,
        MonitorPtr const & monitor, StructureConstPtr const & structure)
{
    longArrayMonitor->status = status;
    longArrayMonitor->monitor = monitor;
    if(!status.isOK())  {
        messagePvt(status.getMessage(),errorMessage);
        longArrayMonitor->event.signal();
        return;
    }
    bool structureOK(true);
    FieldConstPtr field = structure->getField("timeStamp");
    if(field==NULL) structureOK = false;
    field = structure->getField("value");
    if(field==NULL) {
         structureOK = false;
    } else {
        if(field->getType()!=scalarArray) {
            structureOK = false; 
        } else {
            ScalarArrayConstPtr scalarArray = dynamic_pointer_cast<const ScalarArray>(field);
            if(scalarArray->getElementType()!=pvLong) structureOK = false;
        }
    }
    if(!structureOK) {
        String message("monitorConnect: illegal structure");
        messagePvt(message,errorMessage);
        longArrayMonitor->status = Status(Status::STATUSTYPE_ERROR,message);
    }
    longArrayMonitor->event.signal();
}

void LAMMonitorRequester::run()
{
    PVLongArrayPtr pvValue;
    PVTimeStamp pvTimeStamp;
    TimeStamp timeStamp;
    TimeStamp timeStampLast;
    timeStampLast.getCurrent();
    while(true) {
        event.wait();
        if(isDestroyed) {
            runReturned = true;
            return;
        }
        while(true) {
            MonitorElementPtr monitorElement;
            PVStructurePtr pvStructure;
            {
                 Lock xx(mutex);
                 monitorElement = longArrayMonitor->monitor->poll();
                 if(monitorElement!=NULL) pvStructure = monitorElement->pvStructurePtr;
            }
            if(monitorElement==NULL) break;
            pvTimeStamp.attach(pvStructure->getSubField("timeStamp"));
            pvTimeStamp.get(timeStamp);
            pvValue = dynamic_pointer_cast<PVLongArray>(pvStructure->getSubField("value"));
            shared_vector<const int64> data = pvValue->view();
            if(data.size()>0) {
                int64 first = data[0];
                int64 last = data[data.size()-1];
                int64 sum = 0;
                for(size_t i=0; i<data.size(); ++i) sum += data[i];
                double diff = TimeStamp::diff(timeStamp,timeStampLast);
                double elementsPerSecond = data.size();
                elementsPerSecond = 1e-6*elementsPerSecond/diff;
                cout << "first " << first << " last " << last << " sum " << sum;
                cout << " elements/sec " << elementsPerSecond << "million";
                BitSetPtr changed = monitorElement->changedBitSet;
                BitSetPtr overrun = monitorElement->overrunBitSet;
                String buffer;
                changed->toString(&buffer);
                cout << " changed " << buffer;
                buffer.clear();
                overrun->toString(&buffer);
                cout << " overrun " << buffer;
                cout << endl;
                timeStampLast = timeStamp;
            } else {
                cout << "size = 0" << endl;
            }
            longArrayMonitor->monitor->release(monitorElement);
        }
    }
}

void LAMMonitorRequester::monitorEvent(MonitorPtr const & monitor)
{
    event.signal();
}

void LAMMonitorRequester::unlisten(MonitorPtr const & monitor)
{
    messagePvt("unlisten called",errorMessage);
}


LongArrayMonitorPtr LongArrayMonitor::create(
    String const &providerName,
    String const & channelName,
    bool useQueue)
{
    LongArrayMonitorPtr longArrayMonitor(new LongArrayMonitor());
    if(!longArrayMonitor->init(providerName,channelName,useQueue)) longArrayMonitor.reset();
    return longArrayMonitor;
}

LongArrayMonitor::LongArrayMonitor() {}

LongArrayMonitor::~LongArrayMonitor() {}

bool LongArrayMonitor::init(
    String const &providerName,
    String const &channelName,
    bool useQueue)
{
    channelRequester = LAMChannelRequesterPtr(new LAMChannelRequester(getPtrSelf()));
    monitorRequester = LAMMonitorRequesterPtr(new LAMMonitorRequester(getPtrSelf()));
    monitorRequester->init();
    ChannelProvider::shared_pointer channelProvider = getChannelAccess()->getProvider(providerName);
    if(channelProvider==NULL) {
        cout << "provider " << providerName << " not found" << endl;
        return false;
    }
    channel = channelProvider->createChannel(channelName,channelRequester,0);
    event.wait();
    if(!status.isOK()) return false;
    String queueSize("0");
    if(useQueue) queueSize="2";
    String request("record[queueSize=");
    request += queueSize;
    request += "]field(value,timeStamp,alarm)";
    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    PVStructurePtr pvRequest = createRequest->createRequest(request);
    if(pvRequest==NULL) {
        cout << "request logic error " << createRequest->getMessage() << endl;
        return false;
    }
    monitor = channel->createMonitor(monitorRequester,pvRequest);
    event.wait();
    if(!status.isOK()) return false;
    return true;
}

void LongArrayMonitor::start()
{
    monitor->start();
}

void LongArrayMonitor::stop()
{
    monitor->stop();
}

void LongArrayMonitor::destroy()
{
    monitorRequester->destroy();
    monitorRequester.reset();
    monitor->destroy();
    monitor.reset();
    channel->destroy();
    channel.reset();
    channelRequester->destroy();
    channelRequester.reset();
}

}}


