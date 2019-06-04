/* numericRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#include <pv/pvDatabase.h>
#include <pv/convert.h>
#include <pv/standardField.h>
#include <pv/controlSupport.h>
#include <pv/scalarAlarmSupport.h>

#define epicsExportSharedSymbols

#include <pv/numericRecord.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase { 

NumericRecord::~NumericRecord()
{
cout << "NumericRecord::~NumericRecord()\n";
}

NumericRecordPtr NumericRecord::create(
    std::string const & recordName,epics::pvData::ScalarType scalarType)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StandardFieldPtr standardField = getStandardField();
    StructureConstPtr  topStructure = fieldCreate->createFieldBuilder()->
        add("value",scalarType) ->
        add("reset",pvBoolean) ->
        add("alarm",standardField->alarm()) ->
        add("timeStamp",standardField->timeStamp()) ->
        add("display",standardField->display()) ->
        add("control",standardField->control()) ->
        add("scalarAlarm",ScalarAlarmSupport::scalarAlarm()) ->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    NumericRecordPtr pvRecord(
        new NumericRecord(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

NumericRecord::NumericRecord(
    std::string const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

bool NumericRecord::init()
{
    initPVRecord();
    PVRecordPtr pvRecord = shared_from_this();
    PVStructurePtr pvStructure(getPVStructure());
    controlSupport = ControlSupport::create(pvRecord);
    bool result = controlSupport->init(
       pvStructure->getSubField("value"),pvStructure->getSubField("control"));
    if(!result) return false;
    scalarAlarmSupport = ScalarAlarmSupport::create(pvRecord);
    result = scalarAlarmSupport->init(
       pvStructure->getSubField("value"),
       pvStructure->getSubField<PVStructure>("alarm"),
       pvStructure->getSubField("scalarAlarm"));
    if(!result) return false;
    pvReset = getPVStructure()->getSubField<PVBoolean>("reset");
    return true;
}

void NumericRecord::process()
{
    if(pvReset->get()==true) {
        pvReset->put(false);
        controlSupport->reset();
        scalarAlarmSupport->reset();
    } else {
        controlSupport->process();
        scalarAlarmSupport->process();
    }
    PVRecord::process();
}


}}

