/* recordListTest.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */
#ifndef RECORDLIST_H
#define RECORDLIST_H

#include <shareLib.h>

#include <pv/pvDatabase.h>

namespace epics { namespace pvDatabase { 

class RecordListRecord;
typedef std::tr1::shared_ptr<RecordListRecord> RecordListRecordPtr;

class epicsShareClass RecordListRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(RecordListRecord);
    static RecordListRecordPtr create(
        epics::pvData::String const & recordName);
    virtual ~RecordListRecord();
    virtual void destroy();
    virtual bool init();
    virtual void process();
private:
    RecordListRecord(epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStringPtr database;
    epics::pvData::PVStringPtr regularExpression;
    epics::pvData::PVStringPtr status;
    epics::pvData::PVStringArrayPtr names;
};

}}

#endif  /* RECORDLIST_H */
