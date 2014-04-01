/* pvCopyMonitor.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#ifndef PVCOPYMONITOR_H
#define PVCOPYMONITOR_H
#include <string>
#include <stdexcept>
#include <memory>

#include <shareLib.h>

#include <pv/pvCopy.h>
#include <pv/pvAccess.h>
#include <pv/pvDatabase.h>

namespace epics { namespace pvDatabase { 

class PVCopyMonitor;
typedef std::tr1::shared_ptr<PVCopyMonitor> PVCopyMonitorPtr;
class PVCopyMonitorRequester;
typedef std::tr1::shared_ptr<PVCopyMonitorRequester> PVCopyMonitorRequesterPtr;

class epicsShareClass PVCopyMonitor :
    public PVListener,
    public epics::pvData::PVCopyTraverseMasterCallback,
    public std::tr1::enable_shared_from_this<PVCopyMonitor>
{
public:
    POINTER_DEFINITIONS(PVCopyMonitor);
    static PVCopyMonitorPtr create(
        PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester,
        PVRecordPtr const &pvRecord,
        epics::pvData::PVCopyPtr const & pvCopy);
    virtual ~PVCopyMonitor();
    virtual void destroy();
    void startMonitoring(
        epics::pvData::BitSetPtr const &changeBitSet,
        epics::pvData::BitSetPtr const &overrunBitSet);
    void stopMonitoring();
    void switchBitSets(
        epics::pvData::BitSetPtr const &newChangeBitSet,
        epics::pvData::BitSetPtr const &newOverrunBitSet);
    // following are PVListener methods
    virtual void detach(PVRecordPtr const & pvRecord);
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField);
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField);
    virtual void beginGroupPut(PVRecordPtr const & pvRecord);
    virtual void endGroupPut(PVRecordPtr const & pvRecord);
    virtual void unlisten(PVRecordPtr const & pvRecord);
    // following is PVCopyTraverseMasterCallback method
    virtual void nextMasterPVField(epics::pvData::PVFieldPtr const &pvField);
private:
    PVCopyMonitorPtr getPtrSelf()
    {
        return shared_from_this();
    }
    PVCopyMonitor(
        PVRecordPtr const &pvRecord,
        epics::pvData::PVCopyPtr const &pvCopy,
        PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester);
    PVRecordPtr pvRecord;
    epics::pvData::PVCopyPtr pvCopy;
    PVCopyMonitorRequesterPtr pvCopyMonitorRequester;
    epics::pvData::BitSetPtr changeBitSet;
    epics::pvData::BitSetPtr overrunBitSet;
    bool isGroupPut;
    bool dataChanged;
    bool isMonitoring;
    epics::pvData::Mutex mutex;
};

class epicsShareClass PVCopyMonitorRequester
{
public:
    POINTER_DEFINITIONS(PVCopyMonitorRequester);
    virtual ~PVCopyMonitorRequester() {}
    virtual void dataChanged() = 0;
    virtual void unlisten() = 0;
};

}}

#endif  /* PVCOPYMONITOR_H */
