/* exampleLink.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.08.02
 */
#ifndef EXAMPLELINK_H
#define EXAMPLELINK_H

#ifdef epicsExportSharedSymbols
#   define examplelinkEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

//#include <shareLib.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>
#include <pv/pvDatabase.h>
#include <pv/pvCopy.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>


#ifdef examplelinkEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef examplelinkEpicsExportSharedSymbols
#   include <shareLib.h>
#endif


namespace epics { namespace pvDatabase { 


class ExampleLink;
typedef std::tr1::shared_ptr<ExampleLink> ExampleLinkPtr;

class epicsShareClass ExampleLink :
    public PVRecord,
    public epics::pvAccess::ChannelRequester,
    public epics::pvAccess::ChannelGetRequester
{
public:
    POINTER_DEFINITIONS(ExampleLink);
    static ExampleLinkPtr create(
        epics::pvData::String const & recordName,
        epics::pvData::String const & providerName,
        epics::pvData::String const & channelName
        );
    virtual ~ExampleLink() {}
    virtual void destroy();
    virtual bool init();
    virtual void process();
    virtual void channelCreated(
        const epics::pvData::Status& status,
        epics::pvAccess::Channel::shared_pointer const & channel);
    virtual void channelStateChange(
        epics::pvAccess::Channel::shared_pointer const & channel,
        epics::pvAccess::Channel::ConnectionState connectionState);
    virtual void channelGetConnect(
        const epics::pvData::Status& status,
        epics::pvAccess::ChannelGet::shared_pointer const & channelGet,
        epics::pvData::PVStructure::shared_pointer const & pvStructure,
        epics::pvData::BitSet::shared_pointer const & bitSet);
    virtual void getDone(const epics::pvData::Status& status);
    virtual epics::pvData::String getRequesterName() {return channelName;}
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType)
        {
           std::cout << "Why is ExampleLink::message called\n";
        }
private:
    ExampleLink(epics::pvData::String const & recordName,
        epics::pvData::String providerName,
        epics::pvData::String channelName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::String providerName;
    epics::pvData::String channelName;
    epics::pvData::ConvertPtr convert;
    epics::pvData::PVDoubleArrayPtr pvValue;
    epics::pvData::PVTimeStamp pvTimeStamp;
    epics::pvData::TimeStamp timeStamp;
    epics::pvData::PVAlarm pvAlarm;
    epics::pvData::Alarm alarm;
    epics::pvAccess::Channel::shared_pointer channel;
    epics::pvAccess::ChannelGet::shared_pointer channelGet;
    epics::pvData::Event event;
    epics::pvData::Status status;
    epics::pvData::PVStructurePtr getPVStructure;
    epics::pvData::BitSetPtr bitSet;
    epics::pvData::PVDoubleArrayPtr getPVValue;
};

}}

#endif  /* EXAMPLELINK_H */
