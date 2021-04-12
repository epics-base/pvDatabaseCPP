/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2021.04.11
 */
#ifndef PVDBCRSCALARARRAY_H
#define PVDBCRSCALARARRAY_H

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>
#include <pv/pvStructureCopy.h>

#include <shareLib.h>

namespace epics { namespace pvDatabase {

class PvdbcrScalarArray;
typedef std::tr1::shared_ptr<PvdbcrScalarArray> PvdbcrScalarArrayPtr;

/**
 * @brief  PvdbcrScalarArray creates a record with a scalar array value, alarm, and timeStamp.
 *
 */
class epicsShareClass PvdbcrScalarArray :
     public PVRecord
{
private:
  PvdbcrScalarArray(
    std::string const & recordName,epics::pvData::PVStructurePtr const & pvStructure,
    int asLevel,std::string const & asGroup);
public:
    POINTER_DEFINITIONS(PvdbcrScalarArray);
    /**
     * The Destructor.
     */
    virtual ~PvdbcrScalarArray() {}
    /**
     * @brief Create a record.
     *
     * @param recordName The record name.
     * @param scalarType The type for the value field
     * @param asLevel  The access security level.
     * @param asGroup  The access security group.
     * @return The PVRecord
     */
     static PvdbcrScalarArrayPtr create(
        std::string const & recordName,std::string const &  scalarType,
        int asLevel=0,std::string const & asGroup = std::string("DEFAULT"));
};

}}

#endif  /* PVDBCRSCALARARRAY_H */
