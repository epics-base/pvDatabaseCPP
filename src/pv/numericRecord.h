/* numericRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#ifndef NUMERICRECORD_H
#define NUMERICRECORD_H

#include <shareLib.h>

#include <pv/channelProviderLocal.h>
#include <pv/controlSupport.h>

namespace epics { namespace pvDatabase { 


class NumericRecord;
typedef std::tr1::shared_ptr<NumericRecord> NumericRecordPtr;

/**
 * @brief support for control and valueAlarm for a numeric scalar record
 *
 * This is support for a record with a top level field that has type scalar.
 * It provides support for control and valueAlarm
 */
class epicsShareClass NumericRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(NumericRecord);
    /**
     * Factory methods to create NumericRecord.
     * @param recordName The name for the NumericRecord.
     * @param scalarType The scalar type. It must be a numeric type.
     * @return A shared pointer to NumericRecord..
     */
    static NumericRecordPtr create(
        std::string const & recordName,epics::pvData::ScalarType scalarType);
    /**
     * standard init method required by PVRecord
     * @return true unless record name already exists.
     */
    virtual bool init();
    /**
     * @brief Remove the record specified by  recordName.
     */
    virtual void process();
    ~NumericRecord();
private:
    NumericRecord(
        std::string const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    ControlSupportPtr controlSupport;
    epics::pvData::PVBooleanPtr pvReset;
};

}}

#endif  /* NUMERICRECORD_H */
