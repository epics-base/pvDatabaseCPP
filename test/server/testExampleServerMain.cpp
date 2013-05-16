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

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/exampleCounter.h>
#include <pv/powerSupplyRecordTest.h>
#include <pv/channelProviderLocal.h>
#include <pv/recordList.h>

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
    PVDatabasePtr master = PVDatabase::getMaster();
    ChannelProviderLocalPtr channelProvider = getChannelProviderLocal();
    StandardPVFieldPtr standardPVField = getStandardPVField();
    String properties;
    ScalarType scalarType;
    PVRecordPtr pvRecord;
    String recordName;
    bool result(false);
    recordName = "exampleCounter";
    pvRecord = ExampleCounter::create(recordName);
    result = master->addRecord(pvRecord);
    properties = "alarm,timeStamp";
    scalarType = pvDouble;
    recordName = "exampleDouble";
    PVStructurePtr pvStructure;
    pvStructure = standardPVField->scalar(scalarType,properties);
    pvRecord = PVRecord::create(recordName,pvStructure);
    {
        pvRecord->lock_guard();
        pvRecord->process();
    }
    result = master->addRecord(pvRecord);
    recordName = "examplePowerSupply";
    pvStructure = createPowerSupply();
    PowerSupplyRecordTestPtr psr =
        PowerSupplyRecordTest::create(recordName,pvStructure);
    if(psr.get()==NULL) {
        cout << "PowerSupplyRecordTest::create failed" << endl;
        return 1;
    }
    result = master->addRecord(psr);
    recordName = "laptoprecordListPGRPC";
    pvRecord = RecordListRecord::create(recordName);
    result = master->addRecord(pvRecord);
    cout << "exampleServer\n";
    PVStringArrayPtr pvNames = master->getRecordNames();
    String buffer;
    pvNames->toString(&buffer);
    cout << "recordNames" << endl << buffer << endl;
    string str;
    while(true) {
        cout << "Type exit to stop: \n";
        getline(cin,str);
        if(str.compare("exit")==0) break;

    }
    return 0;
}

