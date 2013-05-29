/* powerSupplyRecordTest.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */
#ifndef POWERSUPPLYRECORDTEST_H
#define POWERSUPPLYRECORDTEST_H

#include <pv/pvDatabase.h>
#include <pv/timeStamp.h>
#include <pv/alarm.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvAlarm.h>

namespace epics { namespace pvDatabase { 


class PowerSupplyRecordTest;
typedef std::tr1::shared_ptr<PowerSupplyRecordTest> PowerSupplyRecordTestPtr;

class PowerSupplyRecordTest :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(PowerSupplyRecordTest);
    static PowerSupplyRecordTestPtr create(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    virtual ~PowerSupplyRecordTest();
    virtual void destroy();
    virtual bool init();
    virtual void process();
    void put(double power,double voltage);
    double getPower();
    double getVoltage();
    double getCurrent();
private:
    PowerSupplyRecordTest(epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVDoublePtr pvCurrent;
    epics::pvData::PVDoublePtr pvPower;
    epics::pvData::PVDoublePtr pvVoltage;
    epics::pvData::PVAlarm pvAlarm;
    epics::pvData::PVTimeStamp pvTimeStamp;
    epics::pvData::Alarm alarm;
    epics::pvData::TimeStamp timeStamp;
};

PowerSupplyRecordTestPtr PowerSupplyRecordTest::create(
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
{
    PowerSupplyRecordTestPtr pvRecord(
        new PowerSupplyRecordTest(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

PowerSupplyRecordTest::PowerSupplyRecordTest(
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

PowerSupplyRecordTest::~PowerSupplyRecordTest()
{
}

void PowerSupplyRecordTest::destroy()
{
    PVRecord::destroy();
}

bool PowerSupplyRecordTest::init()
{
    initPVRecord();
    epics::pvData::PVStructurePtr pvStructure = getPVStructure();
    epics::pvData::PVFieldPtr pvField;
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
    epics::pvData::String name;
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

void PowerSupplyRecordTest::process()
{
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
    double voltage = pvVoltage->get();
    double power = pvPower->get();
    if(voltage<1e-3 && voltage>-1e-3) {
        alarm.setMessage("bad voltage");
        alarm.setSeverity(epics::pvData::majorAlarm);
        pvAlarm.set(alarm);
        return;
    }
    double current = power/voltage;
    pvCurrent->put(current);
    alarm.setMessage("");
    alarm.setSeverity(epics::pvData::noAlarm);
    pvAlarm.set(alarm);
}

void PowerSupplyRecordTest::put(double power,double voltage)
{
    pvPower->put(power);
    pvVoltage->put(voltage);
}

double PowerSupplyRecordTest::getPower()
{
    return pvPower->get();
}

double PowerSupplyRecordTest::getVoltage()
{
    return pvVoltage->get();
}

double PowerSupplyRecordTest::getCurrent()
{
    return pvCurrent->get();
}


}}

#endif  /* POWERSUPPLYRECORDTEST_H */
