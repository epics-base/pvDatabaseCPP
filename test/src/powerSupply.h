/* powerSupply.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */
#ifndef POWERSUPPLY_H
#define POWERSUPPLY_H

//#include <shareLib.h>

#ifdef epicsExportSharedSymbols
#   define powersupplyEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvDatabase.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>


#ifdef powersupplyEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef powersupplyEpicsExportSharedSymbols
#   include <shareLib.h>
#endif


namespace epics { namespace pvDatabase { 

epicsShareExtern epics::pvData::PVStructurePtr createPowerSupply();

class epicsShareClass PowerSupply;
typedef std::tr1::shared_ptr<PowerSupply> PowerSupplyPtr;

class PowerSupply :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(PowerSupply);
    static PowerSupplyPtr create(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    virtual ~PowerSupply();
    virtual void destroy();
    virtual bool init();
    virtual void process();
    void put(double power,double voltage);
    double getPower();
    double getVoltage();
    double getCurrent();
private:
    PowerSupply(epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVDoublePtr pvCurrent;
    epics::pvData::PVDoublePtr pvPower;
    epics::pvData::PVDoublePtr pvVoltage;
    epics::pvData::PVAlarm pvAlarm;
    epics::pvData::PVTimeStamp pvTimeStamp;
    epics::pvData::Alarm alarm;
    epics::pvData::TimeStamp timeStamp;
};


}}

#endif  /* POWERSUPPLY_H */
