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
#include "pv/pvdbcrProcessRecord.h"


using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace std;

static const iocshArg testArg0 = { "recordName", iocshArgString };
static const iocshArg testArg1 = { "delay", iocshArgDouble };
static const iocshArg testArg2 = { "asLevel", iocshArgInt };
static const iocshArg *testArgs[] = {&testArg0,&testArg1,&testArg2};

static const iocshFuncDef pvdbcrProcessRecordFuncDef = {"pvdbcrProcessRecord", 3,testArgs};

static void pvdbcrProcessRecordCallFunc(const iocshArgBuf *args)
{
    char *recordName = args[0].sval;
    if(!recordName) {
        throw std::runtime_error("pvdbcrProcessRecordCreate invalid number of arguments");
    }
    double delay = args[1].dval;
    int asLevel = args[2].ival;
    if(delay<0.0) delay = 1.0;
    PvdbcrProcessRecordPtr record = PvdbcrProcessRecord::create(recordName,delay);
    record->setAsLevel(asLevel);
    bool result = PVDatabase::getMaster()->addRecord(record);
    if(!result) cout << "recordname" << " not added" << endl;
}

static void pvdbcrProcessRecordRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&pvdbcrProcessRecordFuncDef, pvdbcrProcessRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(pvdbcrProcessRecordRegister);
}
