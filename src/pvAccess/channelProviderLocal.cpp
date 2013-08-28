/* channelChannelProviderLocal.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */

#include <pv/serverContext.h>
#include <pv/channelProviderLocal.h>
#include <pv/traceRecord.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::cout;
using std::endl;

static String providerName("local");

class LocalChannelProviderFactory;
typedef std::tr1::shared_ptr<LocalChannelProviderFactory> LocalChannelProviderFactoryPtr;

class LocalChannelProviderFactory : public ChannelProviderFactory
{
    
public:
    POINTER_DEFINITIONS(LocalChannelProviderFactory);
    virtual String getFactoryName() { return providerName;}
    static LocalChannelProviderFactoryPtr create(
        ChannelProviderLocalPtr const &channelProvider)
    {
        LocalChannelProviderFactoryPtr xxx(
            new LocalChannelProviderFactory(channelProvider));
        registerChannelProviderFactory(xxx);
        return xxx;
    }
    virtual  ChannelProvider::shared_pointer sharedInstance()
    {
        return channelProvider;
    }
    virtual  ChannelProvider::shared_pointer newInstance()
    {
        return channelProvider;
    }
private:
    LocalChannelProviderFactory(
        ChannelProviderLocalPtr const &channelProvider)
    : channelProvider(channelProvider)
    {}
    ChannelProviderLocalPtr channelProvider;
};

ChannelProviderLocalPtr getChannelProviderLocal()
{
    static ChannelProviderLocalPtr channelProviderLocal;
    static Mutex mutex;
    Lock xx(mutex);
    if(channelProviderLocal.get()==NULL) {
        channelProviderLocal = ChannelProviderLocalPtr(
            new ChannelProviderLocal());
        LocalChannelProviderFactory::create(channelProviderLocal);
    }
    return channelProviderLocal;
}

ChannelProviderLocal::ChannelProviderLocal()
: pvDatabase(PVDatabase::getMaster()),
  beingDestroyed(false)
{
}

ChannelProviderLocal::~ChannelProviderLocal()
{
    cout << "~ChannelProviderLocal()" << endl;
}

void ChannelProviderLocal::destroy()
{
    cout << "ChannelProviderLocal::destroy()" << endl;
    Lock xx(mutex);
    if(beingDestroyed) return;
    beingDestroyed = true;
    pvDatabase->destroy();
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
    PVRecordPtr pvRecord = pvDatabase->findRecord(channelName);
    if(pvRecord.get()!=NULL) {
        ChannelLocalPtr channel(new ChannelLocal(
            getPtrSelf(),channelRequester,pvRecord));
        channelRequester->channelCreated(
            Status::Ok,
            channel);
        pvRecord->addPVRecordClient(channel);
        return channel;
    }   
    Status notFoundStatus(Status::STATUSTYPE_ERROR,String("pv not found"));
    channelRequester->channelCreated(
        notFoundStatus,
        Channel::shared_pointer());
    return Channel::shared_pointer();
}

}}
