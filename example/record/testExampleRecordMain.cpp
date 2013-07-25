/*testExampleRecordMain.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include <epicsExport.h>

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/pvData.h>
#include <pv/pvAccess.h>
#include <pv/powerSupplyRecordTest.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;


static PVStructurePtr createPowerSupply()
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


int main(int argc,char *argv[])
{
    StandardPVFieldPtr standardPVField = getStandardPVField();
    String properties;
    ScalarType scalarType;
    String recordName;
    properties = "alarm,timeStamp";
    scalarType = pvDouble;
    recordName = "exampleDouble";
    PVStructurePtr pvStructure;
    pvStructure = standardPVField->scalar(scalarType,properties);
    PVRecordPtr pvRecord = PVRecord::create(recordName,pvStructure);
    {
        pvRecord->lock_guard();
        pvRecord->process();
    }
    cout << "processed exampleDouble "  << endl;
    pvRecord->destroy();
    recordName = "powerSupplyExample";
    pvStructure.reset();
    pvStructure = createPowerSupply();
    PowerSupplyRecordTestPtr psr =
        PowerSupplyRecordTest::create(recordName,pvStructure);
    if(psr.get()==NULL) {
        cout << "PowerSupplyRecordTest::create failed" << endl;
        return 1;
    }
    double voltage,power,current;
    {
        psr->lock_guard();
        voltage = psr->getVoltage();
        power = psr->getPower();
        current = psr->getCurrent();
    }
    cout << "initial ";
    cout << " voltage " << voltage ;
    cout << " power " << power;
    cout <<  " current " << current;
    cout << endl;
    voltage = 1.0;
    power = 1.0;
    cout << "before put ";
    cout << " voltage " << voltage ;
    cout << " power " << power;
    cout << endl;
    {
        psr->lock_guard();
        psr->put(power,voltage);
        psr->process();
    }
    {
        psr->lock_guard();
        cout << "after put ";
        cout << " voltage " << psr->getVoltage() ;
        cout << " power " << psr->getPower();
        cout <<  " current " << psr->getCurrent();
        cout << endl;
    }
    PVDatabasePtr pvDatabase = PVDatabase::getMaster();
    pvDatabase->addRecord(psr);
    pvDatabase->destroy();
    return 0;
}

