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

#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/channelProviderLocal.h"
#include "pv/pvDatabase.h"
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace std;

class RemoveRecord;
typedef std::tr1::shared_ptr<RemoveRecord> RemoveRecordPtr;


class epicsShareClass RemoveRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(RemoveRecord);
    static RemoveRecordPtr create(
        std::string const & recordName);
    virtual bool init();
    virtual void process();
private:
    RemoveRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
};

RemoveRecordPtr RemoveRecord::create(
    std::string const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr  topStructure = fieldCreate->createFieldBuilder()->
        addNestedStructure("argument")->
            add("recordName",pvString)->
            endNested()->
        addNestedStructure("result") ->
            add("status",pvString) ->
            endNested()->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    RemoveRecordPtr pvRecord(
        new RemoveRecord(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

RemoveRecord::RemoveRecord(
    std::string const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

bool RemoveRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    pvRecordName = pvStructure->getSubField<PVString>("argument.recordName");
    if(!pvRecordName) return false;
    pvResult = pvStructure->getSubField<PVString>("result.status");
    if(!pvResult) return false;
    return true;
}

void RemoveRecord::process()
{
    string name = pvRecordName->get();
    PVRecordPtr pvRecord = PVDatabase::getMaster()->findRecord(name);
    if(!pvRecord) {
        pvResult->put(name + " not found");
        return;
    }
    pvRecord->remove();
    pvResult->put("success");
}

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg *args[] = {&arg0};

static const iocshFuncDef removeRecordFuncDef = {"removeRecordCreate", 1,args};

static void removeRecordCallFunc(const iocshArgBuf *args)
{
    cerr << "DEPRECATED use pvdbcrRemoveRecord instead\n";
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("removeRecord recordName not specified");
    }
    string recordName = string(sval);
    RemoveRecordPtr record = RemoveRecord::create(recordName);
    PVDatabasePtr master = PVDatabase::getMaster();
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void removeRecordRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&removeRecordFuncDef, removeRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(removeRecordRegister);
}
