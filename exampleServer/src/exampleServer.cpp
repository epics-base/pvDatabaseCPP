/* exampleServer.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */

#include <pv/standardPVField.h>

#define epicsExportSharedSymbols

#include <exampleServer.h>

namespace epics { namespace exampleServer { 
using namespace epics::pvData;
using namespace epics::pvDatabase;
using std::tr1::static_pointer_cast;


ExampleServerPtr ExampleServer::create(
    String const & recordName)
{
    StandardPVFieldPtr standardPVField = getStandardPVField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    PVStructurePtr pvArgument = standardPVField->scalar(pvString,"");
    PVStructurePtr pvResult = standardPVField->scalar(pvString,"timeStamp");
    StringArray names;
    names.reserve(2);
    PVFieldPtrArray fields;
    fields.reserve(2);
    names.push_back("argument");
    fields.push_back(pvArgument);
    names.push_back("result");
    fields.push_back(pvResult);
    PVStructurePtr pvStructure = pvDataCreate->createPVStructure(names,fields);
    ExampleServerPtr pvRecord(
        new ExampleServer(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

ExampleServer::ExampleServer(
    String const & recordName,
    PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

ExampleServer::~ExampleServer()
{
}

void ExampleServer::destroy()
{
    PVRecord::destroy();
}

bool ExampleServer::init()
{
    
    initPVRecord();
    PVFieldPtr pvField;
    pvArgumentValue = getPVStructure()->getStringField("argument.value");
    if(pvArgumentValue.get()==NULL) return false;
    pvResultValue = getPVStructure()->getStringField("result.value");
    if(pvResultValue.get()==NULL) return false;
    pvTimeStamp.attach(getPVStructure()->getSubField("result.timeStamp"));
    return true;
}

void ExampleServer::process()
{
    pvResultValue->put(String("Hello ") + pvArgumentValue->get());
    timeStamp.getCurrent();
    pvTimeStamp.set(timeStamp);
}

}}
