/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2019.06.01
 */
#ifndef PVSUPPORT_H
#define PVSUPPORT_H

#ifdef epicsExportSharedSymbols
#   define pvdatabaseEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <list>
#include <map>

#include <pv/pvData.h>
#include <pv/pvTimeStamp.h>
#include <pv/rpcService.h>


#ifdef pvdatabaseEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef pvdatabaseEpicsExportSharedSymbols
#endif

#include <shareLib.h>
#include <pv/pvStructureCopy.h>

namespace epics { namespace pvDatabase { 

class PVSupport;
typedef std::tr1::shared_ptr<PVSupport> PVSupportPtr;

/**
 * @brief Base interface for a PVSupport.
 *
 */
class epicsShareClass PVSupport 
{
public:
    POINTER_DEFINITIONS(PVSupport);
    /**
     * The Destructor.
     */
    virtual ~PVSupport(){}
    /**
     * @brief Optional  initialization method.
     *
     * A derived method <b>Must</b> call initPVSupport.
     * @return <b>true</b> for success and <b>false</b> for failure.
     */
    virtual bool init() {return true;}
    /**
     *  @brief Optional method for derived class.
     *
     * It is called before record is added to database.
     */
    virtual void start() {}
    /**
     * @brief Optional method for derived class.
     *
     *  It is the method that makes a record smart.
     *  If it encounters errors it should raise alarms and/or
     *  call the <b>message</b> method provided by the base class.
     *  If the pvStructure has a top level timeStamp,
     *  the base class sets the timeStamp to the current time.
     */
    virtual void process() = 0;
    /**
     *  @brief Optional method for derived class.
     *
     */
    virtual void reset() {};
};

}}

#endif  /* PVSUPPORT_H */

