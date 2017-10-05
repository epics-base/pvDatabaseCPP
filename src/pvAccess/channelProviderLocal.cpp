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

#include <epicsThread.h>


#include <pv/serverContext.h>
#include <pv/syncChannelFind.h>

#define epicsExportSharedSymbols

#include <pv/channelProviderLocal.h>
#include <pv/traceRecord.h>

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;
using std::string;

namespace epics { namespace pvDatabase { 

class LocalChannelProviderFactory;
typedef std::tr1::shared_ptr<LocalChannelProviderFactory> LocalChannelProviderFactoryPtr;

static string providerName("local");
static ChannelProviderLocalPtr channelProvider;

class LocalChannelProviderFactory : public ChannelProviderFactory
{  
public:
    POINTER_DEFINITIONS(LocalChannelProviderFactory);
    virtual string getFactoryName() { return providerName;}
    virtual  ChannelProvider::shared_pointer sharedInstance()
    {
        if(!channelProvider) channelProvider = ChannelProviderLocalPtr(new ChannelProviderLocal());
        return channelProvider;
    }
    virtual  ChannelProvider::shared_pointer newInstance()
    {
cout << "LocalChannelProviderFactory::newInstance()\n";
        throw std::logic_error("newInstance not Implemented");
    }
};


ChannelProviderLocalPtr getChannelProviderLocal()
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        ChannelProviderFactory::shared_pointer factory(
             new  LocalChannelProviderFactory());
         ChannelProviderRegistry::servers()->add(factory);
    }
    ChannelProvider::shared_pointer channelProvider =
        ChannelProviderRegistry::servers()->getProvider(providerName);
    return std::tr1::dynamic_pointer_cast<ChannelProviderLocal>(channelProvider);
}

ChannelProviderLocal::ChannelProviderLocal()
: pvDatabase(PVDatabase::getMaster()),
  traceLevel(0)
{
}

ChannelProviderLocal::~ChannelProviderLocal()
{
    if(traceLevel>0) {
        cout << "ChannelProviderLocal::~ChannelProviderLocal() \n";
    }
}

std::tr1::shared_ptr<ChannelProvider> ChannelProviderLocal::getChannelProvider()
{
    return shared_from_this();
}

void ChannelProviderLocal::cancel()
{
}


string ChannelProviderLocal::getProviderName()
{
    return providerName;
}

ChannelFind::shared_pointer ChannelProviderLocal::channelFind(
    string const & channelName,
    ChannelFindRequester::shared_pointer  const &channelFindRequester)
{
    if(traceLevel>1) {
        cout << "ChannelProviderLocal::channelFind " << "channelName" << endl;
    }
    Lock xx(mutex);
    PVRecordPtr pvRecord = pvDatabase->findRecord(channelName);
    if(pvRecord) {
        channelFindRequester->channelFindResult(
            Status::Ok,
            shared_from_this(),
            true);
        
    } else {
        Status notFoundStatus(Status::STATUSTYPE_ERROR,"pv not found");
        channelFindRequester->channelFindResult(
            notFoundStatus,
            shared_from_this(),
            false);
    }
    return shared_from_this();
}

ChannelFind::shared_pointer ChannelProviderLocal::channelList(
    ChannelListRequester::shared_pointer const & channelListRequester)
{
    if(traceLevel>1) {
        cout << "ChannelProviderLocal::channelList\n";
    }
    PVStringArrayPtr records;
    {
        Lock guard(mutex);
        records = pvDatabase->getRecordNames();
    }

    channelListRequester->channelListResult(Status::Ok, shared_from_this(), records->view(), false);
    return shared_from_this();
}

Channel::shared_pointer ChannelProviderLocal::createChannel(
    string const & channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority)
{
    if(traceLevel>1) {
        cout << "ChannelProviderLocal::createChannel " << "channelName" << endl;
    }
    Lock xx(mutex);
    PVRecordPtr pvRecord = pvDatabase->findRecord(channelName);
    if(pvRecord) {
        ChannelLocalPtr channel(new ChannelLocal(
            shared_from_this(),channelRequester,pvRecord));
        channelRequester->channelCreated(
            Status::Ok,
            channel);
        pvRecord->addPVRecordClient(channel);
        return channel;
    }   
    Status notFoundStatus(Status::STATUSTYPE_ERROR,"pv not found");
    channelRequester->channelCreated(
        notFoundStatus,
        Channel::shared_pointer());
    return Channel::shared_pointer();
    
}

Channel::shared_pointer ChannelProviderLocal::createChannel(
    string const & channelName,
    ChannelRequester::shared_pointer  const &channelRequester,
    short priority,
    string const &address)
{
    if(!address.empty()) throw std::invalid_argument("address not allowed for local implementation");
    return createChannel(channelName, channelRequester, priority);
}

}}
