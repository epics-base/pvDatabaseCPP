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

// The following must be the last include for code pvDatabase uses
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/pvDatabase.h"
#include "pv/pvdbcrAddRecord.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace std;

static const iocshArg testArg0 = { "recordName", iocshArgString };
static const iocshArg testArg1 = { "asLevel", iocshArgInt };
static const iocshArg *testArgs[] = {&testArg0,&testArg1};

static const iocshFuncDef pvdbcrAddRecordFuncDef = {"pvdbcrAddRecord", 2,testArgs};

static void pvdbcrAddRecordCallFunc(const iocshArgBuf *args)
{
    char *recordName = args[0].sval;
    if(!recordName) {
        throw std::runtime_error("pvdbcrAddRecord invalid number of arguments");
    }
    int asLevel = args[1].ival;
    PvdbcrAddRecordPtr record = PvdbcrAddRecord::create(recordName);
    record->setAsLevel(asLevel);
    bool result = PVDatabase::getMaster()->addRecord(record);
    if(!result) cout << "recordname" << " not added" << endl;
}

static void pvdbcrAddRecordRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdbcrAddRecordFuncDef, pvdbcrAddRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(pvdbcrAddRecordRegister);
}
