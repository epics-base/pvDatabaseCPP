/* channelProviderLocal.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#ifndef CHANNELPROVIDERLOCAL_H
#define CHANNELPROVIDERLOCAL_H
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <set>

#include <shareLib.h>

#include <pv/lock.h>
#include <pv/pvType.h>
#include <pv/pvData.h>
#include <pv/monitorPlugin.h>
#include <pv/pvCopy.h>
#include <pv/pvAccess.h>
#include <pv/pvDatabase.h>
#include <pv/status.h>


namespace epics { namespace pvDatabase { 


class MonitorFactory;
typedef std::tr1::shared_ptr<MonitorFactory> MonitorFactoryPtr;

class MonitorLocal;
typedef std::tr1::shared_ptr<MonitorLocal> MonitorLocalPtr;


class ChannelProviderLocal;
typedef std::tr1::shared_ptr<ChannelProviderLocal> ChannelProviderLocalPtr;
class ChannelLocal;
typedef std::tr1::shared_ptr<ChannelLocal> ChannelLocalPtr;

epicsShareExtern MonitorFactoryPtr getMonitorFactory();

class epicsShareClass MonitorFactory 
{
public:
    POINTER_DEFINITIONS(MonitorFactory);
    virtual ~MonitorFactory();
    virtual void destroy();
    epics::pvData::MonitorPtr createMonitor(
        PVRecordPtr const & pvRecord,
        epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
        epics::pvData::PVStructurePtr const & pvRequest);
private:
    MonitorFactory();
    friend class MonitorLocal;
    friend MonitorFactoryPtr getMonitorFactory();
    bool isDestroyed;
    epics::pvData::Mutex mutex;
};


epicsShareExtern ChannelProviderLocalPtr getChannelProviderLocal();

class epicsShareClass ChannelProviderLocal :
    public epics::pvAccess::ChannelProvider,
    public std::tr1::enable_shared_from_this<ChannelProviderLocal>
{
public:
    POINTER_DEFINITIONS(ChannelProviderLocal);
    virtual ~ChannelProviderLocal();
    virtual void destroy();
    virtual  epics::pvData::String getProviderName();
    virtual epics::pvAccess::ChannelFind::shared_pointer channelFind(
        epics::pvData::String const &channelName,
        epics::pvAccess::ChannelFindRequester::shared_pointer const & channelFindRequester);
    virtual epics::pvAccess::ChannelFind::shared_pointer channelList(
        epics::pvAccess::ChannelListRequester::shared_pointer const & channelListRequester);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority,
        epics::pvData::String const &address);
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelProviderLocal();
    friend ChannelProviderLocalPtr getChannelProviderLocal();
    PVDatabasePtr pvDatabase;
    epics::pvData::Mutex mutex;
    bool beingDestroyed;
    epics::pvAccess::ChannelFind::shared_pointer channelFinder;
    friend class ChannelProviderLocalRun;
};

class epicsShareClass ChannelLocal :
  public epics::pvAccess::Channel,
  public PVRecordClient,
  public std::tr1::enable_shared_from_this<ChannelLocal>
{
public:
    POINTER_DEFINITIONS(ChannelLocal);
    ChannelLocal(
        ChannelProviderLocalPtr const &channelProvider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester,
        PVRecordPtr const & pvRecord
    );
    virtual ~ChannelLocal();
    virtual void destroy();
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    virtual epics::pvAccess::ChannelProvider::shared_pointer getProvider()
    {
        return provider;
    }
    virtual epics::pvData::String getRemoteAddress();
    virtual epics::pvAccess::Channel::ConnectionState getConnectionState();
    virtual epics::pvData::String getChannelName();
    virtual epics::pvAccess::ChannelRequester::shared_pointer getChannelRequester();
    virtual bool isConnected();
    virtual void getField(
        epics::pvAccess::GetFieldRequester::shared_pointer const &requester,
        epics::pvData::String const & subField);
    virtual epics::pvAccess::AccessRights getAccessRights(
        epics::pvData::PVField::shared_pointer const &pvField);
    virtual epics::pvAccess::ChannelProcess::shared_pointer createChannelProcess(
        epics::pvAccess::ChannelProcessRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual epics::pvAccess::ChannelGet::shared_pointer createChannelGet(
        epics::pvAccess::ChannelGetRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual epics::pvAccess::ChannelPut::shared_pointer createChannelPut(
        epics::pvAccess::ChannelPutRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual epics::pvAccess::ChannelPutGet::shared_pointer createChannelPutGet(
        epics::pvAccess::ChannelPutGetRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual epics::pvAccess::ChannelRPC::shared_pointer createChannelRPC(
        epics::pvAccess::ChannelRPCRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual epics::pvData::Monitor::shared_pointer createMonitor(
        epics::pvData::MonitorRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual epics::pvAccess::ChannelArray::shared_pointer createChannelArray(
        epics::pvAccess::ChannelArrayRequester::shared_pointer const &requester,
        epics::pvData::PVStructurePtr const &pvRequest);
    virtual void printInfo();
    virtual void printInfo(epics::pvData::StringBuilder out);
    virtual void detach(PVRecordPtr const &pvRecord);
protected:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
private:
    ChannelProviderLocalPtr provider;
    epics::pvAccess::ChannelRequester::shared_pointer requester;
    PVRecordPtr pvRecord;
    bool beingDestroyed;
    epics::pvData::Mutex mutex;
};

    

}}
#endif  /* CHANNELPROVIDERLOCAL_H */
