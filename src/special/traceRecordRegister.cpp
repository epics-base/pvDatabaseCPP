/*
 * Copyright information and license terms for this software can be
 * found in the file LICENSE that is included with the distribution
 */

/**
 * @author mrk
 * @date 2021.03.12
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

// The following must be the last include
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/channelProviderLocal.h"
#include "pv/pvDatabase.h"

using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace std;

class TraceRecord;
typedef std::tr1::shared_ptr<TraceRecord> TraceRecordPtr;

class epicsShareClass TraceRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(TraceRecord);
    static TraceRecordPtr create(
        std::string const & recordName);
    virtual bool init();
    virtual void process();
private:
    TraceRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVIntPtr pvLevel;
    epics::pvData::PVStringPtr pvResult;
}; 

TraceRecordPtr TraceRecord::create(
    std::string const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr  topStructure = fieldCreate->createFieldBuilder()->
        addNestedStructure("argument")->
            add("recordName",pvString)->
            add("level",pvInt)->
            endNested()->
        addNestedStructure("result") ->
            add("status",pvString) ->
            endNested()->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    TraceRecordPtr pvRecord(
        new TraceRecord(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

TraceRecord::TraceRecord(
    std::string const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}


bool TraceRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    pvRecordName = pvStructure->getSubField<PVString>("argument.recordName");
    if(!pvRecordName) return false;
    pvLevel = pvStructure->getSubField<PVInt>("argument.level");
    if(!pvLevel) return false;
    pvResult = pvStructure->getSubField<PVString>("result.status");
    if(!pvResult) return false;
    return true;
}

void TraceRecord::process()
{
    string name = pvRecordName->get();
    PVRecordPtr pvRecord = PVDatabase::getMaster()->findRecord(name);
    if(!pvRecord) {
        pvResult->put(name + " not found");
        return;
    }
    pvRecord->setTraceLevel(pvLevel->get());
    pvResult->put("success");
}

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg *args[] = {&arg0};

static const iocshFuncDef traceRecordFuncDef = {"traceRecordCreate", 1,args};

static void traceRecordCallFunc(const iocshArgBuf *args)
{
    cerr << "DEPRECATED use pvdbcrTraceRecord instead\n";
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("traceRecord recordName not specified");
    }
    string recordName = string(sval);
    TraceRecordPtr record = TraceRecord::create(recordName);
    PVDatabasePtr master = PVDatabase::getMaster();
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void traceRecordRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&traceRecordFuncDef, traceRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(traceRecordRegister);
}
