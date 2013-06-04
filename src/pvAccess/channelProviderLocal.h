/* channelProviderLocal.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @data 2013.04
 */
#ifndef CHANNELPROVIDERLOCAL_H
#define CHANNELPROVIDERLOCAL_H
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <set>

#include <pv/lock.h>
#include <pv/pvType.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/pvDatabase.h>
#include <pv/status.h>
#include <pv/pvCopy.h>
#include <pv/monitorAlgorithm.h>


namespace epics { namespace pvDatabase { 

class ChannelLocalTrace;
typedef std::tr1::shared_ptr<ChannelLocalTrace> ChannelLocalTracePtr;

class MonitorFactory;
typedef std::tr1::shared_ptr<MonitorFactory> MonitorFactoryPtr;

class MonitorLocal;
typedef std::tr1::shared_ptr<MonitorLocal> MonitorLocalPtr;


class ChannelProviderLocal;
typedef std::tr1::shared_ptr<ChannelProviderLocal> ChannelProviderLocalPtr;
class ChannelLocal;
typedef std::tr1::shared_ptr<ChannelLocal> ChannelLocalPtr;
typedef std::multiset<ChannelLocalPtr> ChannelLocalList;

extern MonitorFactoryPtr getMonitorFactory();
class MonitorLocal;
typedef std::tr1::shared_ptr<MonitorLocal> MonitorLocalPtr;
typedef std::multiset<MonitorLocalPtr> MonitorLocalList;

class ChannelLocalTrace {
public:
    ChannelLocalTrace()
    : channelLocalTraceLevel(0)
    {}
    ~ChannelLocalTrace(){}
    void setLevel(int level)
    {
        channelLocalTraceLevel = level;
    }
    int getLevel()
    {
        return channelLocalTraceLevel;
    }
private:
    int channelLocalTraceLevel;
};

class MonitorFactory 
{
public:
    POINTER_DEFINITIONS(MonitorFactory);
    virtual ~MonitorFactory();
    virtual void destroy();
    epics::pvData::MonitorPtr createMonitor(
        PVRecordPtr const & pvRecord,
        epics::pvData::MonitorRequester::shared_pointer const & monitorRequester,
        epics::pvData::PVStructurePtr const & pvRequest,
        ChannelLocalTracePtr const &channelLocalTrace);
    void registerMonitorAlgorithmCreate(
        MonitorAlgorithmCreatePtr const &monitorAlgorithmCreate);
    MonitorAlgorithmCreatePtr getMonitorAlgorithmCreate(
        epics::pvData::String algorithmName);
private:
    MonitorFactory();
    friend class MonitorLocal;
    friend MonitorFactoryPtr getMonitorFactory();
    std::multiset<MonitorAlgorithmCreatePtr> monitorAlgorithmCreateList;
    std::multiset<MonitorLocalPtr> monitorLocalList;
    bool isDestroyed;
    epics::pvData::Mutex mutex;
};


extern ChannelProviderLocalPtr getChannelProviderLocal();

class ChannelProviderLocal :
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
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority);
    virtual epics::pvAccess::Channel::shared_pointer createChannel(
        epics::pvData::String const &channelName,
        epics::pvAccess::ChannelRequester::shared_pointer const &channelRequester,
        short priority,
        epics::pvData::String const &address);
    void removeChannel(
        epics::pvAccess::Channel::shared_pointer const &channel);
    void createChannelLocalTraceRecord(epics::pvData::String const &recordName);
    ChannelLocalTracePtr getChannelLocalTrace() { return channelLocalTrace;}
private:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
    ChannelProviderLocal();
    friend ChannelProviderLocalPtr getChannelProviderLocal();
    PVDatabasePtr pvDatabase;
    ChannelLocalList channelList;
    epics::pvData::Mutex mutex;
    bool beingDestroyed;
    ChannelLocalTracePtr channelLocalTrace;
    friend class ChannelProviderLocalRun;
};

class ChannelLocal :
  public epics::pvAccess::Channel,
  public PVRecordClient,
  public std::tr1::enable_shared_from_this<ChannelLocal>
{
public:
    POINTER_DEFINITIONS(ChannelLocal);
    ChannelLocal(
        ChannelProviderLocalPtr const &channelProvider,
        epics::pvAccess::ChannelRequester::shared_pointer const & requester,
        PVRecordPtr const & pvRecord,
        ChannelLocalTracePtr const &channelLocalTrace
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
    // following called by derived classes
    void addChannelProcess(epics::pvAccess::ChannelProcess::shared_pointer const &);
    void addChannelGet(epics::pvAccess::ChannelGet::shared_pointer const &);
    void addChannelPut(epics::pvAccess::ChannelPut::shared_pointer const &);
    void addChannelPutGet(epics::pvAccess::ChannelPutGet::shared_pointer const &);
    void addChannelRPC(epics::pvAccess::ChannelRPC::shared_pointer const &);
    void addChannelArray(epics::pvAccess::ChannelArray::shared_pointer const &);
    void removeChannelProcess(epics::pvAccess::ChannelProcess::shared_pointer const &);
    void removeChannelGet(epics::pvAccess::ChannelGet::shared_pointer const &);
    void removeChannelPut(epics::pvAccess::ChannelPut::shared_pointer const &);
    void removeChannelPutGet(epics::pvAccess::ChannelPutGet::shared_pointer const &);
    void removeChannelRPC(epics::pvAccess::ChannelRPC::shared_pointer const &);
    void removeChannelArray(epics::pvAccess::ChannelArray::shared_pointer const &);
protected:
    shared_pointer getPtrSelf()
    {
        return shared_from_this();
    }
private:
    ChannelProviderLocalPtr provider;
    epics::pvAccess::ChannelRequester::shared_pointer requester;
    PVRecordPtr pvRecord;
    ChannelLocalTracePtr channelLocalTrace;
    bool beingDestroyed;
    std::multiset<epics::pvAccess::ChannelProcess::shared_pointer> channelProcessList;
    std::multiset<epics::pvAccess::ChannelGet::shared_pointer> channelGetList;
    std::multiset<epics::pvAccess::ChannelPut::shared_pointer> channelPutList;
    std::multiset<epics::pvAccess::ChannelPutGet::shared_pointer> channelPutGetList;
    std::multiset<epics::pvAccess::ChannelRPC::shared_pointer> channelRPCList;
    std::multiset<epics::pvAccess::ChannelArray::shared_pointer> channelArrayList;
    epics::pvData::Mutex mutex;
};

    

}}
#endif  /* CHANNELPROVIDERLOCAL_H */