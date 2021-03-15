/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 * @date 2021.03.12
 */


/* Author: Marty Kraimer */

#include <epicsThread.h>
#include <iocsh.h>
#include <pv/event.h>
#include <pv/pvAccess.h>
#include <pv/serverContext.h>
#include <pv/pvData.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>
#include <pv/ntscalar.h>

// The following must be the last include for code pvDatabase uses
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/pvDatabase.h"

using namespace epics::pvData;
using namespace epics::nt;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace std;

static const iocshArg testArg0 = { "recordName", iocshArgString };
static const iocshArg testArg1 = { "scalarType", iocshArgString };
static const iocshArg testArg2 = { "asLevel", iocshArgInt };
static const iocshArg *testArgs[] = {&testArg0,&testArg1,&testArg2};

static const iocshFuncDef pvdbcrScalarFuncDef = {"pvdbcrScalar", 3,testArgs};

static void pvdbcrScalarCallFunc(const iocshArgBuf *args)
{
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrScalarCreate recordName not specified");
    }
    string recordName = string(sval);
    sval = args[1].sval;
    if(!sval) {
        throw std::runtime_error("pvdbcrScalarCreate scalarType not specified");
    }
    string scalarType = string(sval);
    int asLevel = args[2].ival;
    try {
        ScalarType st = epics::pvData::ScalarTypeFunc::getScalarType(scalarType);
        NTScalarBuilderPtr ntScalarBuilder = NTScalar::createBuilder();
        PVStructurePtr pvStructure = ntScalarBuilder->
            value(st)->
            addAlarm()->
            addTimeStamp()->
            createPVStructure();
        PVRecordPtr record = PVRecord::create(recordName,pvStructure);
        record->setAsLevel(asLevel);
        PVDatabasePtr master = PVDatabase::getMaster();
        bool result =  master->addRecord(record);
        if(!result) cout << "recordname " << recordName << " not added" << endl;
    } catch(std::exception& ex) {
        cerr << "failure " << ex.what() << "/n";
    }
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
