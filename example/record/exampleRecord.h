/* exampleRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef EXAMPLE_RECORD_H
#define EXAMPLE_RECORD_H
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>

#include <pv/pvDatabase.h>

namespace epics { namespace pvDatabase { 

class ExampleRecord;

class ExampleRecord :
  public virtual PVRecord
{
public:
    POINTER_DEFINITIONS(ExampleRecord);
    static PVRecordPtr create(epics::pvData::String const & recordName);
    virtual ~ExampleRecord();
    virtual bool isSynchronous();
    virtual void process(
        epics::pvDatabase::RecordProcessRequesterPtr const &processRequester);
private:
    ExampleRecord(epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure,
        epics::pvData::PVLongPtr const &pvValue);
    epics::pvData::PVLongPtr pvValue;
};

}}

#endif  /* EXAMPLE_RECORD_H */
