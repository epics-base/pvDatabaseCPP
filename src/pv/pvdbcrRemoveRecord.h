/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.04.11
 */
#ifndef PVDBCRREMOVEARRAY_H
#define PVDBCRREMOVEARRAY_H

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PvdbcrRemoveRecord;
typedef std::tr1::shared_ptr<PvdbcrRemoveRecord> PvdbcrRemoveRecordPtr;

/**
 * @brief  PvdbcrRemoveRecord A record that removes a record from the master database.
 *
 */
class epicsShareClass PvdbcrRemoveRecord :
     public PVRecord
{
private:
  PvdbcrRemoveRecord(
    std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup);
    epics::pvData::PVStringPtr pvRecordName;
    epics::pvData::PVStringPtr pvResult;
public:
    POINTER_DEFINITIONS(PvdbcrRemoveRecord);
    /**
     * The Destructor.
     */
    virtual ~PvdbcrRemoveRecord() {}
    /**
     * @brief Create a record.
     *
     * @param recordName The record name.
     * @param asLevel  The access security level.
     * @param asGroup  The access security group.
     * @return The PVRecord
     */
     static PvdbcrRemoveRecordPtr create(
        std::string const & recordName,
        int asLevel=0,std::string const & asGroup = std::string("DEFAULT"));
    /**
     *  @brief a PVRecord method
     * @return success or failure
     */
    virtual bool init();
    /**
     *  @brief process method that removes a record from the master database.
     */
    virtual void process();
};

}}

#endif  /* PVDBCRREMOVEARRAY_H */
