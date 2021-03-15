/* PvdbcrRemoveRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.03.12
 */
#ifndef PVDBCRREMOVERECORD_H
#define PVDBCRREMOVERECORD_H

#include <pv/channelProviderLocal.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {


class PvdbcrRemoveRecord;
typedef std::tr1::shared_ptr<PvdbcrRemoveRecord> PvdbcrRemoveRecordPtr;

/**
 * @brief Remove another record in the same database.
 *
 * A record to remove another record
 * It is meant to be used via a channelPutGet request.
 * The argument has one field: recordName.
 * The result has a field named status.
 */
class epicsShareClass PvdbcrRemoveRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(PvdbcrRemoveRecord);
    /**
     * Factory methods to create PvdbcrRemoveRecord.
     * @param recordName The name for the PvdbcrRemoveRecord.
     * @return A shared pointer to PvdbcrRemoveRecord..
     */
    static PvdbcrRemoveRecordPtr create(
        std::string const & recordName);
    /**
     * standard init method required by PVRecord
     * @return true unless record name already exists.
     */
    virtual bool init();
    /**
     * @brief Remove the record specified by  recordName.
     */
    virtual void process();
private:
    PvdbcrRemoveRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
};

}}

#endif  /* PVDBCRREMOVERECORD_H */
