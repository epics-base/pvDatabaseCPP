/* powerSupply.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */

#include "powerSupply.h"
#include <pv/standardField.h>
#include <pv/standardPVField.h>

namespace epics { namespace pvDatabase { 
using namespace epics::pvData;

PVStructurePtr createPowerSupply()
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    StandardFieldPtr standardField = getStandardField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();

    size_t nfields = 5;
    StringArray names;
    names.reserve(nfields);
    FieldConstPtrArray powerSupply;
    powerSupply.reserve(nfields);
    names.push_back("alarm");
    powerSupply.push_back(standardField->alarm());
    names.push_back("timeStamp");
    powerSupply.push_back(standardField->timeStamp());
    String properties("alarm,display");
    names.push_back("voltage");
    powerSupply.push_back(standardField->scalar(pvDouble,properties));
    names.push_back("power");
    powerSupply.push_back(standardField->scalar(pvDouble,properties));
    names.push_back("current");
    powerSupply.push_back(standardField->scalar(pvDouble,properties));
    return pvDataCreate->createPVStructure(
            fieldCreate->createStructure(names,powerSupply));
}

using namespace epics::pvData;


PowerSupplyPtr PowerSupply::create(
    String const & recordName,
    PVStructurePtr const & pvStructure)
{
    PowerSupplyPtr pvRecord(
        new PowerSupply(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

PowerSupply::PowerSupply(
    String const & recordName,
    PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

PowerSupply::~PowerSupply()
{
}

void PowerSupply::destroy()
{
    PVRecord::destroy();
}

bool PowerSupply::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    PVFieldPtr pvField;
    bool result;
    pvField = pvStructure->getSubField("timeStamp");
    if(pvField.get()==NULL) {
        std::cerr << "no timeStamp" << std::endl;
        return false;
    }
    result = pvTimeStamp.attach(pvField);
    if(!result) {
        std::cerr << "no timeStamp" << std::endl;
        return false;
    }
    pvField = pvStructure->getSubField("alarm");
    if(pvField.get()==NULL) {
        std::cerr << "no alarm" << std::endl;
        return false;
    }
    result = pvAlarm.attach(pvField);
    if(!result) {
        std::cerr << "no alarm" << std::endl;
        return false;
    }
    String name;
    name = "current.value";
    pvField = pvStructure->getSubField(name);
    if(pvField.get()==NULL) {
        name = "current";
        pvField = pvStructure->getSubField(name);
    }
    if(pvField.get()==NULL) {
        std::cerr << "no current" << std::endl;
        return false;
    }
    pvCurrent = pvStructure->getDoubleField(name);
    if(pvCurrent.get()==NULL) return false;
    name = "voltage.value";
    pvField = pvStructure->getSubField(name);
    if(pvField.get()==NULL) {
        name = "voltage";
        pvField = pvStructure->getSubField(name);
    }
    if(pvField.get()==NULL) {
        std::cerr << "no voltage" << std::endl;
        return false;
    }
    pvVoltage = pvStructure->getDoubleField(name);
    if(pvVoltage.get()==NULL) return false;
    name = "power.value";
    pvField = pvStructure->getSubField(name);
    if(pvField.get()==NULL) {
        name = "power";
        pvField = pvStructure->getSubField(name);
    }
    if(pvField.get()==NULL) {
        std::cerr << "no power" << std::endl;
        return false;
    }
    pvPower = pvStructure->getDoubleField(name);
    if(pvPower.get()==NULL) return false;
    return true;
}

void PowerSupply::process()
{
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
    double voltage = pvVoltage->get();
    double power = pvPower->get();
    if(voltage<1e-3 && voltage>-1e-3) {
        alarm.setMessage("bad voltage");
        alarm.setSeverity(majorAlarm);
        pvAlarm.set(alarm);
        return;
    }
    double current = power/voltage;
    pvCurrent->put(current);
    alarm.setMessage("");
    alarm.setSeverity(noAlarm);
    pvAlarm.set(alarm);
}

void PowerSupply::put(double power,double voltage)
{
    pvPower->put(power);
    pvVoltage->put(voltage);
}

double PowerSupply::getPower()
{
    return pvPower->get();
}

double PowerSupply::getVoltage()
{
    return pvVoltage->get();
}

double PowerSupply::getCurrent()
{
    return pvCurrent->get();
}


}}
