/*exampleDatabase.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.07.24
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <vector>
#include <iostream>

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/channelProviderLocal.h>
#include <pv/recordList.h>
#include <pv/traceRecord.h>

#include <powerSupply.h>
#include <exampleDatabase.h>

using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;

static FieldCreatePtr fieldCreate = getFieldCreate();
static StandardFieldPtr standardField = getStandardField();
static PVDataCreatePtr pvDataCreate = getPVDataCreate();
static StandardPVFieldPtr standardPVField = getStandardPVField();

static void createStructureArrayRecord(
    PVDatabasePtr const &master,
    ScalarType scalarType,
    String const &recordName)
{
    StructureConstPtr structure = standardField->scalar(
        pvDouble,
        String("value,alarm,timeStamp"));
    StringArray names(2);
    FieldConstPtrArray fields(2);
    names[0] = "timeStamp";
    names[1] = "value";
    fields[0] = standardField->timeStamp();
    fields[1] = fieldCreate->createStructureArray(structure);
    StructureConstPtr top = fieldCreate->createStructure(names,fields);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(top);
    PVRecordPtr pvRecord = PVRecord::create(recordName,pvStructure);
    bool result = master->addRecord(pvRecord);
    if(!result) cout<< "record " << recordName << " not added" << endl;
}

static void createRecords(
    PVDatabasePtr const &master,
    ScalarType scalarType,
    String const &recordNamePrefix,
    String const &properties)
{
    String recordName = recordNamePrefix;
    PVStructurePtr pvStructure = standardPVField->scalar(scalarType,properties);
    PVRecordPtr pvRecord = PVRecord::create(recordName,pvStructure);
    bool result = master->addRecord(pvRecord);
    if(!result) cout<< "record " << recordName << " not added" << endl;
    recordName += "Array";
    pvStructure = standardPVField->scalarArray(scalarType,properties);
    pvRecord = PVRecord::create(recordName,pvStructure);
    result = master->addRecord(pvRecord);
}

void ExampleDatabase::create()
{
    PVDatabasePtr master = PVDatabase::getMaster();
    PVRecordPtr pvRecord;
    String recordName;
    bool result(false);
    recordName = "traceRecordPGRPC";
    pvRecord = TraceRecord::create(recordName);
    result = master->addRecord(pvRecord);
    if(!result) cout<< "record " << recordName << " not added" << endl;
    String properties;
    properties = "alarm,timeStamp";
    createRecords(master,pvBoolean,"exampleBoolean",properties);
    createRecords(master,pvByte,"exampleByte",properties);
    createRecords(master,pvShort,"exampleShort",properties);
    createRecords(master,pvInt,"exampleInt",properties);
    createRecords(master,pvLong,"exampleLong",properties);
    createRecords(master,pvFloat,"exampleFloat",properties);
    createRecords(master,pvDouble,"exampleDouble",properties);
    createRecords(master,pvString,"exampleString",properties);
    createStructureArrayRecord(master,pvDouble,"exampleStructureArray");
    recordName = "examplePowerSupply";
    PVStructurePtr pvStructure = createPowerSupply();
    PowerSupplyPtr psr =
        PowerSupply::create(recordName,pvStructure);
    if(psr.get()==NULL) {
        cout << "PowerSupply::create failed" << endl;
        return;
    }
    result = master->addRecord(psr);
    if(!result) cout<< "record " << recordName << " not added" << endl;
    recordName = "laptoprecordListPGRPC";
    pvRecord = RecordListRecord::create(recordName);
    result = master->addRecord(pvRecord);
    if(!result) cout<< "record " << recordName << " not added" << endl;
}

