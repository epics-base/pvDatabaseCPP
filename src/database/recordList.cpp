/* recordListTest.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */

#include <pv/recordList.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase { 

RecordListRecordPtr RecordListRecord::create(
    epics::pvData::String const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    StringArray argNames(2);
    FieldConstPtrArray argFields(2);
    argNames[0] = "database";
    argFields[0] = fieldCreate->createScalar(pvString);
    argNames[1] = "regularExpression";
    argFields[1] = fieldCreate->createScalar(pvString);
    StringArray resNames(2);
    FieldConstPtrArray resFields(2);
    resNames[0] = "status";
    resFields[0] = fieldCreate->createScalar(pvString);
    resNames[1] = "names";
    resFields[1] = fieldCreate->createScalarArray(pvString);
    StringArray topNames(2);
    FieldConstPtrArray topFields(2);
    topNames[0] = "arguments";
    topFields[0] = fieldCreate->createStructure(argNames,argFields);
    topNames[1] = "result";
    topFields[1] = fieldCreate->createStructure(resNames,resFields);
    StructureConstPtr topStructure =
        fieldCreate->createStructure(topNames,topFields);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(topStructure);
    RecordListRecordPtr pvRecord(
        new RecordListRecord(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

RecordListRecord::RecordListRecord(
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

RecordListRecord::~RecordListRecord()
{
   destroy();
}

void RecordListRecord::destroy()
{
    PVRecord::destroy();
}

bool RecordListRecord::init()
{
    initPVRecord();
    PVStructurePtr pvStructure = getPVStructure();
    database = pvStructure->getStringField("arguments.database");
    if(database.get()==NULL) return false;
    regularExpression = pvStructure->getStringField(
        "arguments.regularExpression");
    if(regularExpression.get()==NULL) return false;
    status = pvStructure->getStringField("result.status");
    if(status.get()==NULL) return false;
    PVFieldPtr pvField = pvStructure->getSubField("result.names");
    if(pvField.get()==NULL) {
        std::cerr << "no result.names" << std::endl;
        return false;
    }
    names = static_pointer_cast<PVStringArray>(
         pvStructure->getScalarArrayField("result.names",pvString));
    if(names.get()==NULL) return false;
    return true;
}

void RecordListRecord::process()
{
    PVStringArrayPtr pvNames = PVDatabase::getMaster()->getRecordNames();
    std::vector<String> const & xxx = pvNames->getVector();
    size_t n = xxx.size();
    names->put(0,n,xxx,0);
    String message("");
    if(database->get().compare("master")!=0) {
        message += " can only access master ";
    }
    String regEx = regularExpression->get();
    if(regEx.compare("")!=0 && regEx.compare(".*")!=0) {
         message += " regularExpression not implemented ";
    }
    status->put(message);
}


}}

