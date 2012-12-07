/* testRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */
#ifndef TEST_RECORD_H
#define TEST_RECORD_H
#include <string>
#include <cstring>
#include <stdexcept>
#include <memory>

#include <pv/pvDatabase.h>

namespace epics { namespace pvDatabase { 

class TestRecord;

class TestRecord :
  public virtual PVRecord
{
public:
    POINTER_DEFINITIONS(TestRecord);
    static PVRecordPtr create(epics::pvData::String const & recordName);
    virtual ~TestRecord();
    virtual bool isSynchronous();
    virtual void process(
        epics::pvDatabase::RecordProcessRequesterPtr const &processRequester,
        bool alreadyLocked);
private:
    TestRecord(epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure,
        epics::pvData::PVLongPtr const &pvValue);
    epics::pvData::PVLongPtr pvValue;
};

}}

#endif  /* TEST_RECORD_H */
