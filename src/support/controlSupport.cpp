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
    }
    if(!pvControl || !pvLimitLow || !pvLimitHigh || !pvMinStep) {
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

void ControlSupport::process()
{
    ConvertPtr convert = getConvert();
    double value = convert->toDouble(pvValue);
    if(value==requestedValue&&value==currentValue) return;
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
}

void ControlSupport::reset()
{
   isMinStep = false;
}


}}

