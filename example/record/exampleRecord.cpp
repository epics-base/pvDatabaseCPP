/* exampleRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
/* Marty Kraimer 2011.03 */
/* This connects to a V3 record and presents the data as a PVStructure
 * It provides access to  value, alarm, display, and control.
 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <memory>

#include <pv/standardPVField.h>
#include "exampleRecord.h"


namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using namespace epics::pvAccess;
using std::tr1::static_pointer_cast;

ExampleRecord::~ExampleRecord(){}

PVRecordPtr ExampleRecord::create(String const & recordName)
{
    String properties;
    PVStructurePtr pvStructure = getStandardPVField()->scalar(pvLong,properties);
    PVLongPtr pvValue =  pvStructure->getLongField("value");
    PVRecordPtr pvRecord(new ExampleRecord(recordName,pvStructure,pvValue));
    pvRecord->init();
    return pvRecord;
}

ExampleRecord::ExampleRecord(
    String const & recordName,
    PVStructurePtr const & pvStructure,
    PVLongPtr const &pvValue)
: PVRecord(recordName,pvStructure),
  pvValue(pvValue)
{}

bool ExampleRecord::isSynchronous() {return true;}

void ExampleRecord::process(
    RecordProcessRequesterPtr const &processRequester,bool alreadyLocked)
{
    if(!alreadyLocked) lock();
    pvValue->put(pvValue->get() + 1);
    processRequester->recordProcessResult(Status::Ok);
    unlock();
    processRequester->recordProcessComplete();
    dequeueProcessRequest(processRequester);
}


}}

