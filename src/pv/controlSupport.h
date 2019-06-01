/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#ifndef CONTROLSUPPORT_H
#define CONTROLSUPPORT_H

#ifdef epicsExportSharedSymbols
#   define pvdatabaseEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <pv/pvDatabase.h>
#include <pv/pvSupport.h>

#ifdef pvdatabaseEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef pvdatabaseEpicsExportSharedSymbols
#endif

#include <shareLib.h>
#include <pv/pvStructureCopy.h>

namespace epics { namespace pvDatabase { 

class ControlSupport;
typedef std::tr1::shared_ptr<ControlSupport> ControlSupportPtr;

/**
 * @brief Base interface for a ControlSupport.
 *
 */
class epicsShareClass ControlSupport :
    PVSupport
{
public:
    POINTER_DEFINITIONS(ControlSupport);
    /**
     * The Destructor.
     */
    virtual ~ControlSupport();
    /**
     * @brief Optional  initialization method.
     *
     * A derived method <b>Must</b> call initControlSupport.
     * @return <b>true</b> for success and <b>false</b> for failure.
     */
    virtual bool init();
    /**
     * @brief Optional method for derived class.
     *
     *  
     */
    virtual void process();
    /**
     *  @brief Optional method for derived class.
     *
     */
    virtual void reset();
    static ControlSupportPtr create(PVRecordPtr const & pvRecord);
private:
    ControlSupport(PVRecordPtr const & pvRecord);
    PVRecordPtr pvRecord;
    epics::pvData::PVScalarPtr pvValue;
    epics::pvData::PVStructurePtr pvControl;
    epics::pvData::PVDoublePtr pvLimitLow;
    epics::pvData::PVDoublePtr pvLimitHigh;
    epics::pvData::PVDoublePtr pvMinStep;
    double requestedValue;
    double currentValue;
    bool isMinStep;
};

}}

#endif  /* CONTROLSUPPORT_H */

