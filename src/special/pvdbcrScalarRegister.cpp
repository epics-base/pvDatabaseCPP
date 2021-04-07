/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 * @date 2021.04.07
 */
#include <iocsh.h>
#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/alarm.h>
#include <pv/pvAlarm.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>

// The following must be the last include for code pvDatabase implements
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvDatabase.h"
using namespace epics::pvData;
using namespace std;

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg arg1 = { "scalarType", iocshArgString };
static const iocshArg arg2 = { "asLevel", iocshArgInt };
static const iocshArg arg3 = { "asGroup", iocshArgString };
static const iocshArg *args[] = {&arg0,&arg1,&arg2,&arg3};

static const iocshFuncDef pvdbcrScalarFuncDef = {"pvdbcrScalar", 4,args};

static void pvdbcrScalarCallFunc(const iocshArgBuf *args)
{
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrScalar recordName not specified");
    }
    string recordName = string(sval);
    sval = args[1].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrScalar scalarType not specified");
    }
    string scalarType = string(sval);
    int asLevel = args[2].ival;
    string asGroup("DEFAULT");
    sval = args[3].sval;
    if(sval) {
        asGroup = string(sval);
    }
    ScalarType st = epics::pvData::ScalarTypeFunc::getScalarType(scalarType);
    FieldCreatePtr fieldCreate = getFieldCreate();
    StandardFieldPtr standardField = getStandardField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr top = fieldCreate->createFieldBuilder()->
        add("value",st) ->
        add("timeStamp",standardField->timeStamp()) ->
        add("alarm",standardField->alarm()) ->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(top);   
    epics::pvDatabase::PVRecordPtr record = epics::pvDatabase::PVRecord::create(recordName,pvStructure);
    record->setAsLevel(asLevel);
    record->setAsGroup(asGroup);
    epics::pvDatabase::PVDatabasePtr master = epics::pvDatabase::PVDatabase::getMaster();
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void pvdbcrScalarRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdbcrScalarFuncDef, pvdbcrScalarCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(pvdbcrScalarRegister);
}
