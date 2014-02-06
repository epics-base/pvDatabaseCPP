/* traceRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */

#include <pv/traceRecord.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase { 

TraceRecordPtr TraceRecord::create(
    epics::pvData::String const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StringArray topNames(2);
    FieldConstPtrArray topFields(2);
    topNames[0] = "argument";
    topNames[1] = "result";
    StringArray argNames(2);
    FieldConstPtrArray argFields(2);
    argNames[0] = "recordName";
    argNames[1] = "level";
    argFields[0] = fieldCreate->createScalar(pvString);
    argFields[1] = fieldCreate->createScalar(pvInt);
    topFields[0] = fieldCreate->createStructure(argNames,argFields);
    StringArray resNames(1);
    FieldConstPtrArray resFields(1);
    resNames[0] = "status";
    resFields[0] = fieldCreate->createScalar(pvString);
    topFields[1] = fieldCreate->createStructure(resNames,resFields);
    StructureConstPtr topStructure =
        fieldCreate->createStructure(topNames,topFields);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    TraceRecordPtr pvRecord(
        new TraceRecord(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

TraceRecord::TraceRecord(
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure),
  pvDatabase(PVDatabase::getMaster()),
  isDestroyed(false)
{
}

TraceRecord::~TraceRecord()
{
}

void TraceRecord::destroy()
{
    PVRecord::destroy();
}

bool TraceRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    pvRecordName = pvStructure->getStringField("argument.recordName");
    if(pvRecordName==NULL) return false;
    pvLevel = pvStructure->getIntField("argument.level");
    if(pvLevel==NULL) return false;
    pvResult = pvStructure->getStringField("result.status");
    if(pvResult==NULL) return false;
    return true;
}

void TraceRecord::process()
{
    String name = pvRecordName->get();
    PVRecordPtr pvRecord = pvDatabase->findRecord(name);
    if(pvRecord==NULL) {
        pvResult->put(name + " not found");
        return;
    }
    pvRecord->setTraceLevel(pvLevel->get());
    pvResult->put("success");
}


}}

