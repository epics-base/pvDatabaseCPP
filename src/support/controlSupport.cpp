/* controlSupport.cpp */
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
#include <pv/pvSupport.h>
#include <pv/convert.h>
#include <pv/standardField.h>

#define epicsExportSharedSymbols

#include <pv/controlSupport.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase { 

ControlSupport::~ControlSupport()
{
cout << "ControlSupport::~ControlSupport()\n";
}

epics::pvData::StructureConstPtr ControlSupport::controlField()
{
    return FieldBuilder::begin()
            ->setId("control_t")
            ->add("limitLow", pvDouble)
            ->add("limitHigh", pvDouble)
            ->add("minStep", pvDouble)
            ->add("outputValue", pvDouble)
            ->createStructure();
}


ControlSupportPtr ControlSupport::create(PVRecordPtr const & pvRecord)
{
   ControlSupportPtr support(new ControlSupport(pvRecord));
   return support;
}

ControlSupport::ControlSupport(PVRecordPtr const & pvRecord)
   : pvRecord(pvRecord)
{}

bool ControlSupport::init(PVFieldPtr const & pv,PVFieldPtr const & pvsup)
{
    if(pv) {
         if(pv->getField()->getType()==epics::pvData::scalar) {
              ScalarConstPtr s = static_pointer_cast<const Scalar>(pv->getField());
              if(ScalarTypeFunc::isNumeric(s->getScalarType())) {
                   pvValue = static_pointer_cast<PVScalar>(pv);
              }
         }
    }
    if(!pvValue) {
        cout << "ControlSupport for record " << pvRecord->getRecordName()
        << " failed because not numeric scalar\n";
        return false;
    }
    pvControl = static_pointer_cast<PVStructure>(pvsup);
    if(pvControl) {
       pvLimitLow = pvControl->getSubField<PVDouble>("limitLow");
       pvLimitHigh = pvControl->getSubField<PVDouble>("limitHigh");
       pvMinStep = pvControl->getSubField<PVDouble>("minStep");
       pvOutputValue = pvControl->getSubField<PVDouble>("outputValue");
    }
    if(!pvControl || !pvLimitLow || !pvLimitHigh || !pvMinStep || !pvOutputValue) {
        cout << "ControlSupport for record " << pvRecord->getRecordName()
        << " failed because pvSupport not a valid control structure\n";
        return false;
    }
    ConvertPtr convert = getConvert();
    requestedValue = convert->toDouble(pvValue);
    currentValue = requestedValue;
    isMinStep = false;
    return true;
}

bool ControlSupport::process()
{
    ConvertPtr convert = getConvert();
    double value = convert->toDouble(pvValue);
    if(value==requestedValue&&value==currentValue) return false;
    if(!isMinStep) requestedValue = value;
    double limitLow = pvLimitLow->get();
    double limitHigh = pvLimitHigh->get();
    double minStep = pvMinStep->get();
    if(limitHigh>limitLow) {
        if(requestedValue>limitHigh) requestedValue = limitHigh;
        if(requestedValue<limitLow) requestedValue = limitLow;
    }
    double diff = requestedValue - currentValue;
    double outputValue = requestedValue;
    if(minStep>0.0) {
        if(diff<0.0) {
            outputValue = currentValue - minStep;
            isMinStep = true;
            if(outputValue<requestedValue) {
                 outputValue = requestedValue;
                 isMinStep = false;
            }
        } else {
            outputValue = currentValue + minStep;
            isMinStep = true;
            if(outputValue>requestedValue)  {
                 outputValue = requestedValue;
                 isMinStep = false;
            }
        }
    }
    currentValue = outputValue;
    pvOutputValue->put(outputValue);
    if(!isMinStep && (outputValue!=requestedValue)) {
         convert->fromDouble(pvValue,requestedValue);
    }
    return true;
}

void ControlSupport::reset()
{
   isMinStep = false;
}


}}

