/* PvdbcrProcessRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.03.12
 */
#ifndef PVDBCRPROCESSRECORD_H
#define PVDBCRPROCESSRECORD_H

#include <map>
#include <epicsThread.h>
#include <pv/event.h>
#include <pv/channelProviderLocal.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

typedef std::tr1::shared_ptr<epicsThread> EpicsThreadPtr;

class PvdbcrProcessRecord;
typedef std::tr1::shared_ptr<PvdbcrProcessRecord> PvdbcrProcessRecordPtr;

/**
 * @brief Process another record in the same database.
 *
 * A record to process another record
 * It is meant to be used via a channelPutGet request.
 * The argument has one field: recordName.
 * The result has a field named status.
 */
class epicsShareClass PvdbcrProcessRecord :
    public PVRecord,
    public epicsThreadRunable
{
public:
    POINTER_DEFINITIONS(PvdbcrProcessRecord);
    /**
     * Factory methods to create ProcessRecord.
     * @param recordName The name for the ProcessRecord.
     * @param delay Delay time to wait between process requests.
     * @return A shared pointer to ProcessRecord.
     */
    static PvdbcrProcessRecordPtr create(
        std::string const & recordName,double delay);
    /**
     * standard init method required by PVRecord
     * @return true unless record name already exists.
     */
    virtual bool init();
    /**
     * @brief Process the record specified by  recordName.
     */
    virtual void process();
    /**
     * @brief The run method for the thread.
     */
    virtual void run();
    /**
     * @brief Start the thread
     */
    void startThread();
    /**
     * @brief Stop the thread
     */
    void stop();
private:
    PvdbcrProcessRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure,double delay);
    double delay;
    EpicsThreadPtr thread;
    epics::pvData::Event runStop;
    epics::pvData::Event runReturn;
    PVDatabasePtr pvDatabase;
    PVRecordMap pvRecordMap;
    epics::pvData::PVStringPtr pvCommand;
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
    epics::pvData::Mutex mutex;
};

}}

#endif  /* PVDBCRPROCESSRECORD_H */
