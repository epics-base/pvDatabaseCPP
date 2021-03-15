/* PvdbcrTraceRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.03.12
 */
#ifndef PVDBCRTRACERECORD_H
#define PVDBCRTRACERECORD_H

#include <pv/channelProviderLocal.h>

#include <shareLib.h>


namespace epics { namespace pvDatabase {


class PvdbcrTraceRecord;
typedef std::tr1::shared_ptr<PvdbcrTraceRecord> PvdbcrTraceRecordPtr;

/**
 * @brief Trace activity of  PVRecord.
 *
 * A record to set the trace value for another record
 * It is meant to be used via a channelPutGet request.
 * The argument has two fields: recordName and  level.
 * The result has a field named status.
 */
class epicsShareClass PvdbcrTraceRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(PvdbcrTraceRecord);
    /**
     * @brief Factory method to create PvdbcrTraceRecord.
     *
     * @param recordName The name for the PvdbcrTraceRecord.
     * @return A shared pointer to PvdbcrTraceRecord.
     */
    static PvdbcrTraceRecordPtr create(
        std::string const & recordName);
    /**
     * standard init method required by PVRecord
     * @return true unless record name already exists.
     */
    virtual bool init();
    /**
     * @brief Set the trace level for record specified by  recordName.
     */
    virtual void process();
private:
    PvdbcrTraceRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVIntPtr pvLevel;
    epics::pvData::PVStringPtr pvResult;
};

}}

#endif  /* PVDBCRTRACERECORD_H */
