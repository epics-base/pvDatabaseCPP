/* channelListTraceRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */

#include <pv/channelLocalTraceRecord.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase { 

ChannelLocalTraceRecordPtr ChannelLocalTraceRecord::create(
    ChannelLocalTracePtr const &channelLocalTrace,
    epics::pvData::String const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StringArray argNames(1);
    FieldConstPtrArray argFields(1);
    argNames[0] = "value";
    argFields[0] = fieldCreate->createScalar(pvInt);
    StructureConstPtr topStructure =
        fieldCreate->createStructure(argNames,argFields);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    ChannelLocalTraceRecordPtr pvRecord(
        new ChannelLocalTraceRecord(channelLocalTrace,recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

ChannelLocalTraceRecord::ChannelLocalTraceRecord(
    ChannelLocalTracePtr const &channelLocalTrace,
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure),
  channelLocalTrace(channelLocalTrace)
{
}

ChannelLocalTraceRecord::~ChannelLocalTraceRecord()
{
}

void ChannelLocalTraceRecord::destroy()
{
    PVRecord::destroy();
}

bool ChannelLocalTraceRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();

    pvValue = pvStructure->getIntField("value");
    if(pvValue==NULL) return false;
    return true;
}

void ChannelLocalTraceRecord::process()
{
    channelLocalTrace->setLevel(pvValue->get());
}


}}

