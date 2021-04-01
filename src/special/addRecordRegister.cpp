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

// The following must be the last include for code exampleLink uses
#include <epicsExport.h>
#define epicsExportSharedSymbols
#include "pv/pvStructureCopy.h"
#include "pv/channelProviderLocal.h"
#include "pv/pvDatabase.h"
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace std;

class AddRecord;
typedef std::tr1::shared_ptr<AddRecord> AddRecordPtr;


class epicsShareClass AddRecord :
    public PVRecord
{
private:
    AddRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
public:
    POINTER_DEFINITIONS(AddRecord);

    static AddRecordPtr create(
        std::string const & recordName);
    virtual bool init();
    virtual void process();
};

AddRecordPtr AddRecord::create(
    std::string const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StructureConstPtr  topStructure = fieldCreate->createFieldBuilder()->
        addNestedStructure("argument")->
            add("recordName",pvString)->
            addNestedUnion("union") ->
                endNested()->
            endNested()->
        addNestedStructure("result") ->
            add("status",pvString) ->
            endNested()->
        createStructure();
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    AddRecordPtr pvRecord(
        new AddRecord(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

AddRecord::AddRecord(
    std::string const & recordName,
    PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

bool AddRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    pvRecordName = pvStructure->getSubField<PVString>("argument.recordName");
    if(!pvRecordName) return false;
    pvResult = pvStructure->getSubField<PVString>("result.status");
    if(!pvResult) return false;
    return true;
}

void AddRecord::process()
{
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    string name = pvRecordName->get();
    PVRecordPtr pvRecord = PVDatabase::getMaster()->findRecord(name);
    if(pvRecord) {
        pvResult->put(name + " already exists");
        return;
    }
    PVUnionPtr pvUnion = getPVStructure()->getSubField<PVUnion>("argument.union");
    if(!pvUnion) {
        pvResult->put(name + " argument.union is NULL");
        return;
    }
    PVFieldPtr pvField(pvUnion->get());
    if(!pvField) {
        pvResult->put(name + " union has no value");
        return;
    }
    if(pvField->getField()->getType()!=epics::pvData::structure) {
        pvResult->put(name + " union most be a structure");
        return;
    }
    StructureConstPtr st = static_pointer_cast<const Structure>(pvField->getField());
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(st);
    PVRecordPtr pvRec = PVRecord::create(name,pvStructure);
    bool result = PVDatabase::getMaster()->addRecord(pvRec);
    if(result) {
        pvResult->put("success");
    } else {
        pvResult->put("failure");
    }
}

static const iocshArg arg0 = { "recordName", iocshArgString };
static const iocshArg *args[] = {&arg0};

static const iocshFuncDef addRecordFuncDef = {"addRecordCreate", 1,args};

static void addRecordCallFunc(const iocshArgBuf *args)
{
    cerr << "DEPRECATED use pvdbcrAddRecord instead\n";
    char *sval = args[0].sval;
    if(!sval) {
        throw std::runtime_error("addRecordCreate recordName not specified");
    }
    string recordName = string(sval);
    AddRecordPtr record = AddRecord::create(recordName);
    PVDatabasePtr master = PVDatabase::getMaster();
    bool result =  master->addRecord(record);
    if(!result) cout << "recordname " << recordName << " not added" << endl;
}

static void addRecordCreateRegister(void)
{
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        iocshRegister(&addRecordFuncDef, addRecordCallFunc);
    }
}

extern "C" {
    epicsExportRegistrar(addRecordCreateRegister);
}
