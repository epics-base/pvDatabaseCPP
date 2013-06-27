/* traceRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */
#ifndef TRACERECORD_H
#define TRACERECORD_H

#include <pv/channelProviderLocal.h>

namespace epics { namespace pvDatabase { 


class TraceRecord;
typedef std::tr1::shared_ptr<TraceRecord> TraceRecordPtr;

class TraceRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(TraceRecord);
    static TraceRecordPtr create(
        epics::pvData::String const & recordName);
    virtual ~TraceRecord();
    virtual void destroy();
    virtual bool init();
    virtual void process();
private:
    TraceRecord(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    PVDatabasePtr pvDatabase;
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVIntPtr pvLevel;
    epics::pvData::PVStringPtr pvResult;
    bool isDestroyed;
};

}}

#endif  /* TRACERECORD_H */
