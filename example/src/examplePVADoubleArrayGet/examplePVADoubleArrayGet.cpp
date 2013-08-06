/* examplePVADoubleArrayGet.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.08.02
 */

#include <pv/examplePVADoubleArrayGet.h>
#include <pv/standardPVField.h>
#include <pv/convert.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::cout;
using std::endl;

ExamplePVADoubleArrayGetPtr ExamplePVADoubleArrayGet::create(
    String const & recordName,
    String const & providerName,
    String const & channelName)
{
    PVStructurePtr pvStructure = getStandardPVField()->scalarArray(
        pvDouble,"alarm.timeStamp");
    ExamplePVADoubleArrayGetPtr pvRecord(
        new ExamplePVADoubleArrayGet(
           recordName,providerName,channelName,pvStructure));    
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

ExamplePVADoubleArrayGet::ExamplePVADoubleArrayGet(
    String const & recordName,
    String providerName,
    String channelName,
    PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure),
  providerName(providerName),
  channelName(channelName),
  convert(getConvert())
{
}

void ExamplePVADoubleArrayGet::destroy()
{
    PVRecord::destroy();
}

bool ExamplePVADoubleArrayGet::init()
{
    initPVRecord();

    PVStructurePtr pvStructure = getPVRecordStructure()->getPVStructure();
    pvTimeStamp.attach(pvStructure->getSubField("timeStamp"));
    pvAlarm.attach(pvStructure->getSubField("alarm"));
    pvValue = static_pointer_cast<PVDoubleArray>(
        pvStructure->getScalarArrayField("value",pvDouble));
    if(pvValue==NULL) {
        return false;
    }
    ChannelAccess::shared_pointer channelAccess = getChannelAccess();
    ChannelProvider::shared_pointer provider =
        channelAccess->getProvider(providerName);
    if(provider==NULL) {
         cout << getRecordName() << " provider "
              << providerName << " does not exist" << endl;
        return false;
    }
    ChannelRequester::shared_pointer channelRequester =
        dynamic_pointer_cast<ChannelRequester>(getPtrSelf());
    channel = provider->createChannel(channelName,channelRequester);
    event.wait();
    if(!status.isOK()) {
        cout << getRecordName() << " createChannel failed "
             << status.getMessage() << endl;
        return false;
    }
    ChannelGetRequester::shared_pointer channelGetRequester =
        dynamic_pointer_cast<ChannelGetRequester>(getPtrSelf());
    PVStructurePtr pvRequest = getCreateRequest()->createRequest(
        "value,alarm,timeStamp",getPtrSelf());
    channelGet = channel->createChannelGet(channelGetRequester,pvRequest);
    event.wait();
    if(!status.isOK()) {
        cout << getRecordName() << " createChannelGet failed "
             << status.getMessage() << endl;
        return false;
    }
    getPVValue = static_pointer_cast<PVDoubleArray>(
        getPVStructure->getScalarArrayField("value",pvDouble));
    if(getPVValue==NULL) {
        cout << getRecordName() << " get value not  PVDoubleArray" << endl;
        return false;
    }
    return true;
}

void ExamplePVADoubleArrayGet::process()
{
    status = Status::Ok;
    channelGet->get(false);
    event.wait();
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
    AlarmSeverity severity(noAlarm);
    if(!status.isOK()) {
        switch(status.getType()) {
        case Status::STATUSTYPE_OK: severity = noAlarm; break;
        case Status::STATUSTYPE_WARNING: severity = minorAlarm; break;
        case Status::STATUSTYPE_ERROR: severity = majorAlarm; break;
        case Status::STATUSTYPE_FATAL: severity = invalidAlarm; break;
        }
        alarm.setSeverity(severity);
    } else {
        convert->copy(getPVValue,pvValue);
    }
    alarm.setMessage(status.getMessage());
    pvAlarm.set(alarm);
}

void ExamplePVADoubleArrayGet::channelCreated(
        const Status& status,
        Channel::shared_pointer const & channel)
{
    this->status = status;
    this->channel = channel;
    event.signal();
}

void ExamplePVADoubleArrayGet::channelStateChange(
        Channel::shared_pointer const & channel,
        Channel::ConnectionState connectionState)
{
}

void ExamplePVADoubleArrayGet::channelGetConnect(
        const Status& status,
        ChannelGet::shared_pointer const & channelGet,
        PVStructure::shared_pointer const & pvStructure,
        BitSet::shared_pointer const & bitSet)
{
    this->status = status;
    this->channelGet = channelGet;
    this->getPVStructure = pvStructure;
    this->bitSet = bitSet;
    event.signal();
}

void ExamplePVADoubleArrayGet::getDone(const Status& status)
{
    this->status = status;
    event.signal();
}

}}
