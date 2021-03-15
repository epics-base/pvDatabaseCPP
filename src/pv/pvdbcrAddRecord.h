/* PvdbcrAddRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.03.12
 */
#ifndef PVDBCRADDRECORD_H
#define PVDBCRADDRECORD_H

#include <pv/channelProviderLocal.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {


class PvdbcrAddRecord;
typedef std::tr1::shared_ptr<PvdbcrAddRecord> PvdbcrAddRecordPtr;

/**
 * @brief Add another record in the same database.
 *
 * A record to add another record
 * It is meant to be used via a channelPutGet request.
 * The argument has one field: recordName.
 * The result has a field named status.
 */
class epicsShareClass PvdbcrAddRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(PvdbcrAddRecord);
    /**
     * Factory methods to create PvdbcrAddRecord.
     * @param recordName The name for the PvdbcrAddRecord.
     * @return A shared pointer to PvdbcrAddRecord.
     */
    static PvdbcrAddRecordPtr create(
        std::string const & recordName);
    /**
     * standard init method required by PVRecord
     * @return true unless record name already exists.
     */
    virtual bool init();
    /**
     * @brief Add the record specified by  recordName.
     */
    virtual void process();
private:
    PvdbcrAddRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
};

}}

#endif  /* PVDBCRADDRECORD_H */
