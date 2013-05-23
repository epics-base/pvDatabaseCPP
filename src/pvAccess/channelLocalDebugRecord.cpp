/* channelListDebugRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */

#include <pv/channelLocalDebugRecord.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase { 

ChannelLocalDebugRecordPtr ChannelLocalDebugRecord::create(
    ChannelLocalDebugPtr const &channelLocalDebug,
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
    ChannelLocalDebugRecordPtr pvRecord(
        new ChannelLocalDebugRecord(channelLocalDebug,recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

ChannelLocalDebugRecord::ChannelLocalDebugRecord(
    ChannelLocalDebugPtr const &channelLocalDebug,
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure),
  channelLocalDebug(channelLocalDebug)
{
}

ChannelLocalDebugRecord::~ChannelLocalDebugRecord()
{
}

void ChannelLocalDebugRecord::destroy()
{
    PVRecord::destroy();
}

bool ChannelLocalDebugRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();

    pvValue = pvStructure->getIntField("value");
    if(pvValue==NULL) return false;
    return true;
}

void ChannelLocalDebugRecord::process()
{
    channelLocalDebug->setLevel(pvValue->get());
}


}}

