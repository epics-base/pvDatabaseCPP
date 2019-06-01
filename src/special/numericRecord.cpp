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
        add("alarm",standardField->alarm()) ->
        add("timeStamp",standardField->timeStamp()) ->
        add("display",standardField->display()) ->
        add("control",standardField->control()) ->
        add("valueAlarm",standardField->doubleAlarm()) ->
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
    PVStructurePtr pvStructure = getPVStructure();
    PVFieldPtr pv(pvStructure->getSubField("value"));
    if(pv) {
         if(pv->getField()->getType()==epics::pvData::scalar) {
              ScalarConstPtr s = static_pointer_cast<const Scalar>(pv->getField());
              if(ScalarTypeFunc::isNumeric(s->getScalarType())) {
                   pvValue = static_pointer_cast<PVScalar>(pv);
              }
         }
    }
    if(!pvValue) {
        cout << "create record " << getRecordName()
        << " failed because not numeric scalar\n";
        return false;
    }
    ConvertPtr convert = getConvert();
    requestedValue = convert->toDouble(pvValue);
    currentValue = requestedValue;
    isMinStep = false;
    pvControl = pvStructure->getSubField<PVStructure>("control");
    pvLimitLow = pvControl->getSubField<PVDouble>("limitLow");
    pvLimitHigh = pvControl->getSubField<PVDouble>("limitHigh");
    pvMinStep = pvControl->getSubField<PVDouble>("minStep");
    pvValueAlarm = pvStructure->getSubField<PVStructure>("valueAlarm");
    return true;
}

void NumericRecord::process()
{
    ConvertPtr convert = getConvert();
    double value = convert->toDouble(pvValue);
cout << "value " << value
<< " requestedValue " << requestedValue
<< " currentValue " << currentValue
<< " isMinStep " << (isMinStep ? "true" : "false")
<< "\n";
    if(value==requestedValue&&value==currentValue) {
        PVRecord::process();
        return;
    }
    if(!isMinStep) requestedValue = value;
    double limitLow = pvLimitLow->get();
    double limitHigh = pvLimitHigh->get();
    double minStep = pvMinStep->get();
    if(limitHigh>limitLow) {
        if(value>limitHigh) value = limitHigh;
        if(value<limitLow) value = limitLow;
        if(!isMinStep) {
            if(requestedValue>limitHigh) requestedValue = limitHigh;
            if(requestedValue<limitLow) requestedValue = limitLow;
        }
    }
    if(minStep>0.0) {
        double diff = requestedValue - currentValue;
        if(diff<0.0) {
            value = currentValue - minStep;
            isMinStep = true;
            if(value<requestedValue) {
                 value = requestedValue;
                 isMinStep = false;
            }
        } else {
            value = currentValue + minStep;
            isMinStep = true;
            if(value>requestedValue)  {
                 value = requestedValue;
                 isMinStep = false;
            }
        }
cout << "diff " << diff
<< " value " << value
<< " isMinStep " << (isMinStep ? "true" : "false")
<< "\n";
    }
    currentValue = value;
    convert->fromDouble(pvValue,value);
    PVRecord::process();
}


}}

