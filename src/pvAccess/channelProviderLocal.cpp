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
#include <pv/channelLocalTraceRecord.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

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
  beingDestroyed(false),
  channelLocalTrace(new ChannelLocalTrace())
{
}

ChannelProviderLocal::~ChannelProviderLocal()
{
    if(channelLocalTrace->getLevel()>0)
    {
        std::cout << "~ChannelProviderLocal()" << std::endl;
    }
}

void ChannelProviderLocal::destroy()
{
    Lock xx(mutex);
    if(channelLocalTrace->getLevel()>0)
    {
        std::cout << "ChannelProviderLocal::destroy";
        std::cout << " destroyed " << beingDestroyed << std::endl;
    }
    if(beingDestroyed) return;
    beingDestroyed = true;
    ChannelLocalList::iterator iter;
    while(true) {
        iter = channelList.begin();
        if(iter==channelList.end()) break;
        (*iter)->destroy();
        channelList.erase(iter);
    }
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
    if(channelLocalTrace->getLevel()>2)
    {
        std::cout << "ChannelProviderLocal::channelFind" << std::endl;
    }
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
    PVRecordPtr pvRecord = pvDatabase->findRecord(channelName);
    if(pvRecord.get()!=NULL) {
        ChannelLocalPtr channel(new ChannelLocal(
            getPtrSelf(),channelRequester,pvRecord,channelLocalTrace));
        channelRequester->channelCreated(
            Status::Ok,
            channel);
        if(channelLocalTrace->getLevel()>1)
        {
            std::cout << "ChannelProviderLocal::createChannel";
            std::cout << " channelName " << channelName << std::endl;
        }
        pvRecord->addPVRecordClient(channel);
        channelList.insert(channel);
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
    if(channelLocalTrace->getLevel()>0)
    {
        std::cout << "ChannelProviderLocal::removeChannel";
        std::cout << " destroyed " << beingDestroyed << std::endl;
    }
    if(beingDestroyed) return;
    ChannelLocalList::iterator iter;
    for(iter = channelList.begin(); iter!=channelList.end(); ++iter)
    {
        if((*iter).get()==channel.get()) {
            if(channelLocalTrace->getLevel()>1)
            {
                std::cout << "ChannelProviderLocal::removeChannel";
                std::cout << " channelName " << channel->getChannelName() << std::endl;
            }
            channelList.erase(iter);
            return;
        }
    }
}

void ChannelProviderLocal::createChannelLocalTraceRecord(
    String const &recordName)
{
    ChannelLocalTraceRecordPtr pvRecord
         = ChannelLocalTraceRecord::create(channelLocalTrace,recordName);
    PVDatabasePtr master = PVDatabase::getMaster();
    bool result = master->addRecord(pvRecord);
    if(!result) {
        cout << "result of addRecord " << recordName << " " << result << endl;
    }
}

}}
