/* channelChannelProviderLocal.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

#include <pv/serverContext.h>
#include <pv/channelProviderLocal.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

static String providerName("local");

class LocalChannelCTX;
typedef std::tr1::shared_ptr<LocalChannelCTX> LocalChannelCTXPtr;
class LocalChannelCTX :
    public epics::pvData::Runnable,
    public std::tr1::enable_shared_from_this<LocalChannelCTX>
{
public:
    POINTER_DEFINITIONS(LocalChannelCTX);
    static LocalChannelCTXPtr create(
        ChannelProviderLocalPtr const &channelProvider);
    ChannelProviderLocalPtr getChannelProviderLocal() {return channelProvider;}
    virtual ~LocalChannelCTX();
    virtual void run();
private:
    LocalChannelCTX(
        ChannelProviderLocalPtr const &channelProvider);
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    epics::pvData::Event event;
    ChannelProviderLocalPtr channelProvider;
    epics::pvAccess::ServerContextImpl::shared_pointer ctx;
    epics::pvData::Thread *thread;
};

LocalChannelCTXPtr LocalChannelCTX::create(
    ChannelProviderLocalPtr const & channelProvider)
{
    static LocalChannelCTXPtr pvServiceChannelCTX;
    static Mutex mutex;
    Lock xx(mutex);

   if(pvServiceChannelCTX.get()==0) {
      pvServiceChannelCTX = LocalChannelCTXPtr(
          new LocalChannelCTX(channelProvider));
   }
   return pvServiceChannelCTX;
}

LocalChannelCTX::LocalChannelCTX(
    ChannelProviderLocalPtr const &channelProvider)
:
  channelProvider(channelProvider),
  ctx(ServerContextImpl::create()),
  thread(new Thread(
      String("pvServiceChannel"),
      lowerPriority,
      this,
      epicsThreadStackBig))
{}

LocalChannelCTX::~LocalChannelCTX()
{
    ctx->shutdown();
    // we need thead.waitForCompletion()
    event.wait();
    epicsThreadSleep(1.0);
    delete thread;
}
void LocalChannelCTX::run()
{
    registerChannelProvider(channelProvider);
    String providerName = channelProvider->getProviderName();
    ctx->setChannelProviderName(providerName);
    ctx->initialize(getChannelAccess());
    ctx->printInfo();
    ctx->run(0);
    ctx->destroy();
    event.signal();
}


ChannelProviderLocalPtr ChannelProviderLocal::create()
{
    ChannelProviderLocalPtr channelProvider(new ChannelProviderLocal());
    LocalChannelCTX::create(channelProvider);
    return channelProvider;
}

ChannelProviderLocal::ChannelProviderLocal()
: pvDatabase(PVDatabase::getMaster()),
  beingDestroyed(false)
{
}

ChannelProviderLocal::~ChannelProviderLocal()
{
    pvDatabase.reset();
}

void ChannelProviderLocal::destroy()
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    unregisterChannelProvider(getPtrSelf());
    beingDestroyed = true;
    ChannelLocalList::iterator iter;
    for(iter = channelList.begin(); iter!=channelList.end(); ++iter) {
        ChannelLocalPtr channel = *iter;
        channel->destroy();
    }
    channelList.clear();
}

String ChannelProviderLocal::getProviderName()
{
    return providerName;
}

ChannelFind::shared_pointer ChannelProviderLocal::channelFind(
    String const & channelName,
    ChannelFindRequester::shared_pointer  const &channelFindRequester)
{
    Lock xx(mutex);
    bool found = false;
    ChannelLocalList::iterator iter;
    for(iter = channelList.begin(); iter!=channelList.end(); ++iter)
    {
        if((*iter)->getChannelName().compare(channelName)==0) {
            found = true;
            break;
        }
    }
    if(found) {
        channelFindRequester->channelFindResult(
            Status::Ok,
            ChannelFind::shared_pointer(),
            true);
    }
    PVRecordPtr pvRecord = pvDatabase->findRecord(channelName);
    if(pvRecord.get()!=NULL) {
        channelFindRequester->channelFindResult(
            Status::Ok,
            ChannelFind::shared_pointer(),
            true);
        
    } else {
        Status notFoundStatus(Status::STATUSTYPE_ERROR,String("pv not found"));
        channelFindRequester->channelFindResult(
            notFoundStatus,
            ChannelFind::shared_pointer(),
            false);
    }
    return ChannelFind::shared_pointer();
}

Channel::shared_pointer ChannelProviderLocal::createChannel(
    String const & channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority)
{
    return createChannel(channelName,channelRequester,priority,"");
}

Channel::shared_pointer ChannelProviderLocal::createChannel(
    String const & channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority,
    String const &address)
{
    Lock xx(mutex);
    ChannelLocalList::iterator iter;
    for(iter = channelList.begin(); iter!=channelList.end(); ++iter)
    {
        if((*iter)->getChannelName().compare(channelName)==0) {
            return *iter;
        }
    }
    PVRecordPtr pvRecord = pvDatabase->findRecord(channelName);
    if(pvRecord.get()!=NULL) {
        Channel::shared_pointer channel(new ChannelLocal(
            getPtrSelf(),channelRequester,pvRecord));
        channelRequester->channelCreated(
            Status::Ok,
            channel);
        return channel;
    }   
    Status notFoundStatus(Status::STATUSTYPE_ERROR,String("pv not found"));
    channelRequester->channelCreated(
        notFoundStatus,
        Channel::shared_pointer());
    return Channel::shared_pointer();
}

void ChannelProviderLocal::removeChannel(
    Channel::shared_pointer const & channel)
{
    Lock xx(mutex);
    if(beingDestroyed) return;
    ChannelLocalList::iterator iter;
    for(iter = channelList.begin(); iter!=channelList.end(); ++iter)
    {
        if((*iter).get()==channel.get()) {
            channelList.erase(iter);
            return;
        }
    }
}

}}
