/* longArrayPut.cpp */
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
#include <pv/caProvider.h>

#include <longArrayPut.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;
using std::ostringstream;

static String requesterName("longArrayPut");
static String request("value");
static epics::pvData::Mutex printMutex;

class LongArrayChannelPut :
    virtual public ChannelRequester,
    virtual public ChannelPutRequester,
    public std::tr1::enable_shared_from_this<LongArrayChannelPut>,
    public epicsThreadRunable
{
public:
    LongArrayChannelPut(
        String providerName,
        String channelName,
        size_t arraySize,
        int iterBetweenCreateChannel,
        int iterBetweenCreateChannelPut,
        double delayTime)
    : providerName(providerName),
      channelName(channelName),
      arraySize(arraySize),
      iterBetweenCreateChannel(iterBetweenCreateChannel),
      iterBetweenCreateChannelPut(iterBetweenCreateChannelPut),
      delayTime(delayTime),
      isDestroyed(false),
      runReturned(false),
      threadName("longArrayPut")
    {}
    virtual ~LongArrayChannelPut(){}
    bool init();
    virtual void destroy();
    virtual void run();
    virtual String getRequesterName() { return requesterName;}
    virtual void message(String const & message, MessageType messageType)
       {
           Lock guard(printMutex);
           cout << requesterName << " message " << message << endl;
       }
    virtual void channelCreated(
        const Status& status,
        Channel::shared_pointer const & channel);
    virtual void channelStateChange(
        Channel::shared_pointer const & channel,
        Channel::ConnectionState connectionState);
    virtual void channelPutConnect(
        Status const & status,
        ChannelPut::shared_pointer const & channelPut,
        PVStructurePtr const &pvStructure,
        BitSetPtr const &bitSet);
    virtual void putDone(Status const & status);
    virtual void getDone(Status const & status){}
private:
    LongArrayChannelPutPtr getPtrSelf()
    {
        return shared_from_this();
    }
    String providerName;
    String channelName;
    size_t arraySize;
    int iterBetweenCreateChannel;
    int iterBetweenCreateChannelPut;
    double delayTime;
    bool isDestroyed;
    bool runReturned;
    epics::pvData::String threadName;
    Status status;
    Event event;
    Mutex mutex;
    std::auto_ptr<epicsThread> thread;
    Channel::shared_pointer channel;
    ChannelPut::shared_pointer channelPut;
    PVStructurePtr pvStructure;
    PVLongArrayPtr pvLongArray;
    BitSetPtr bitSet;
};

bool LongArrayChannelPut::init()
{
    ChannelProvider::shared_pointer channelProvider = getChannelAccess()->getProvider(providerName);
    if(channelProvider==NULL) {
        cout << "provider " << providerName << " not found" << endl;
        return false;
    }
    channel = channelProvider->createChannel(channelName,getPtrSelf(),0);
    event.wait();
    if(!status.isOK()) return false;
    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    PVStructurePtr pvRequest = createRequest->createRequest(request);
    if(pvRequest==NULL) {
        cout << "request logic error " << createRequest->getMessage() << endl;
        return false;
    }
    channelPut = channel->createChannelPut(getPtrSelf(),pvRequest);
    event.wait();
    if(!status.isOK()) return false;
     thread = std::auto_ptr<epicsThread>(new epicsThread(
        *this,
        threadName.c_str(),
        epicsThreadGetStackSize(epicsThreadStackSmall),
        epicsThreadPriorityLow));
     thread->start();
     event.signal();
     return true;
}

void LongArrayChannelPut::destroy()
{
    if(isDestroyed) return;
    isDestroyed = true;
    event.signal();
    while(true) {
        if(runReturned) break;
        epicsThreadSleep(.01);
    }
    thread->exitWait();
    channel->destroy();
    channelPut.reset();
    channel.reset();
}

void LongArrayChannelPut::channelCreated(
    const Status& status,
    Channel::shared_pointer const & channel)
{
    if(!status.isOK()) message(status.getMessage(),errorMessage);
    this->status = status;
    this->channel = channel;
    event.signal();
}

void LongArrayChannelPut::channelStateChange(
    Channel::shared_pointer const & channel,
    Channel::ConnectionState connectionState)
{
   MessageType messageType =
       (connectionState==Channel::CONNECTED ? infoMessage : errorMessage);
   message("channelStateChange",messageType);
}


void LongArrayChannelPut::channelPutConnect(
        Status const & status,
        ChannelPut::shared_pointer const & channelPut,
        PVStructurePtr const &pvStructure,
        BitSetPtr const &bitSet)
{
    this->status = status;
    if(!status.isOK())  {
        message(status.getMessage(),errorMessage);
        event.signal();
        return;
    }
    this->channelPut = channelPut;
    this->pvStructure = pvStructure;
    this->bitSet = bitSet;
    bool structureOK(true);
    PVFieldPtr pvField = pvStructure->getSubField("value");
    if(pvField==NULL) {
         structureOK = false;
    } else {
        FieldConstPtr field = pvField->getField();
        if(field->getType()!=scalarArray) {
            structureOK = false; 
        } else {
            ScalarArrayConstPtr scalarArray = dynamic_pointer_cast<const ScalarArray>(field);
            if(scalarArray->getElementType()!=pvLong) structureOK = false;
        }
    }
    if(!structureOK) {
        String mess("channelPutConnect: illegal structure");
        message(mess,errorMessage);
        this->status = Status(Status::STATUSTYPE_ERROR,mess);
    }
    pvLongArray = static_pointer_cast<PVLongArray>(pvField);
    event.signal();
}

void LongArrayChannelPut::run()
{
    while(true) {
        event.wait();
        if(isDestroyed) {
            runReturned = true;
            return;
        }
        TimeStamp timeStamp;
        TimeStamp timeStampLast;
        timeStampLast.getCurrent();
        int numChannelPut = 0;
        int numChannelCreate = 0;
        size_t nElements = 0;
        while(true) {
            nElements += sizeof(int64) * arraySize;
            shared_vector<int64> xxx(arraySize,numChannelPut);
            shared_vector<const int64> data(freeze(xxx));
            pvLongArray->replace(data);
            bitSet->set(pvLongArray->getFieldOffset());
            channelPut->put(false);
            event.wait();
            if(isDestroyed) {
                runReturned = true;
                return;
            }
            if(delayTime>0.0) epicsThreadSleep(delayTime);
            if(isDestroyed) {
                runReturned = true;
                return;
            }
            timeStamp.getCurrent();
            double diff = TimeStamp::diff(timeStamp,timeStampLast);
            if(diff>=1.0) {
                ostringstream out;
                out << "put numChannelPut " << numChannelPut;
                out << " time " << diff ;
                double elementsPerSec = nElements;
                elementsPerSec /= diff;
                if(elementsPerSec>10.0e9) {
                     elementsPerSec /= 1e9;
                     out << " gigaElements/sec " << elementsPerSec;
                } else if(elementsPerSec>10.0e6) {
                     elementsPerSec /= 1e6;
                     out << " megaElements/sec " << elementsPerSec;
                } else if(elementsPerSec>10.0e3) {
                     elementsPerSec /= 1e3;
                     out << " kiloElements/sec " << elementsPerSec;
                } else  {
                     out << " Elements/sec " << elementsPerSec;
                }
                cout << out.str() << endl;
                timeStampLast = timeStamp;
                nElements = 0;
            }
            ++numChannelCreate;
            bool createPut = false;
            if(iterBetweenCreateChannel!=0) {
                if(numChannelCreate>=iterBetweenCreateChannel) {
                    channel->destroy();
                    epicsThreadSleep(1.0);
                    ChannelProvider::shared_pointer channelProvider =
                         getChannelAccess()->getProvider(providerName);
                    channel = channelProvider->createChannel(
                        channelName,getPtrSelf(),0);
                    event.wait();
                    if(isDestroyed) {
                        runReturned = true;
                        return;
                    }
                    if(!status.isOK()) {
                         message(status.getMessage(),errorMessage);
                         return;
                    }
                    cout<< "createChannel success" << endl;
                    createPut = true;
                    numChannelCreate = 0;
                }
            }
            ++numChannelPut;
            if(iterBetweenCreateChannelPut!=0) {
                if(numChannelPut>=iterBetweenCreateChannelPut) createPut = true;
            }
            if(createPut) {
                 numChannelPut = 0;
                 channelPut->destroy();
                 CreateRequest::shared_pointer createRequest = CreateRequest::create();
                 PVStructurePtr pvRequest = createRequest->createRequest(request);
                 if(pvRequest==NULL) {
                     cout << "request logic error " << createRequest->getMessage() << endl;
                     return ;
                 }
                 channelPut = channel->createChannelPut(getPtrSelf(),pvRequest);
                 event.wait();
                 if(isDestroyed) {
                     runReturned = true;
                     return;
                 }
                 if(!status.isOK()) {
                      message(status.getMessage(),errorMessage);
                      return;
                 }
                 cout<< "createChannelPut success" << endl;
            }
        }
    }
}

void LongArrayChannelPut::putDone(Status const & status)
{
    event.signal();
}


LongArrayPutPtr LongArrayPut::create(
    String const &providerName,
    String const & channelName,
    size_t arraySize,
    int iterBetweenCreateChannel,
    int iterBetweenCreateChannelPut,
    double delayTime)
{
    LongArrayPutPtr longArrayPut(
        new LongArrayPut(
            providerName,
            channelName,
            arraySize,
            iterBetweenCreateChannel,
            iterBetweenCreateChannelPut,
            delayTime));
    if(!longArrayPut->init()) longArrayPut.reset();
    return longArrayPut;
}

LongArrayPut::LongArrayPut(
    String const &providerName,
    String const & channelName,
    size_t arraySize,
    int iterBetweenCreateChannel,
    int iterBetweenCreateChannelPut,
    double delayTime)
: providerName(providerName),
  channelName(channelName),
  arraySize(arraySize),
  iterBetweenCreateChannel(iterBetweenCreateChannel),
  iterBetweenCreateChannelPut(iterBetweenCreateChannelPut),
  delayTime(delayTime)
{}


LongArrayPut::~LongArrayPut() {}

bool LongArrayPut::init()
{
    longArrayChannelPut = LongArrayChannelPutPtr(new LongArrayChannelPut(
        providerName,
        channelName,
        arraySize,
        iterBetweenCreateChannel,
        iterBetweenCreateChannelPut,
        delayTime));
    return  longArrayChannelPut->init();
}

void LongArrayPut::destroy()
{
    longArrayChannelPut->destroy();
    longArrayChannelPut.reset();
}

}}


