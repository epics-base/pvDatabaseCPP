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

#ifdef epicsExportSharedSymbols
#   define pvCopyMonitorEpicsExportSharedSymbols
#   undef epicsExportSharedSymbols
#endif

#include <string>
#include <stdexcept>
#include <memory>
#include <list>

#include <shareLib.h>

#include <pv/monitorPlugin.h>
#include <pv/pvCopy.h>
#include <pv/pvAccess.h>

#ifdef pvCopyMonitorEpicsExportSharedSymbols
#   define epicsExportSharedSymbols
#	undef pvCopyMonitorEpicsExportSharedSymbols
#endif

#include <pv/pvDatabase.h>

namespace epics { namespace pvDatabase { 

class PVCopyMonitor;
typedef std::tr1::shared_ptr<PVCopyMonitor> PVCopyMonitorPtr;
class PVCopyMonitorRequester;
typedef std::tr1::shared_ptr<PVCopyMonitorRequester> PVCopyMonitorRequesterPtr;

struct PVCopyMonitorFieldNode;
typedef std::tr1::shared_ptr<PVCopyMonitorFieldNode> PVCopyMonitorFieldNodePtr;


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
    void startMonitoring();
    void stopMonitoring();
    void setMonitorElement(epics::pvData::MonitorElementPtr const &monitorElement);
    void monitorDone(epics::pvData::MonitorElementPtr const &monitorElement);
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
    void init(epics::pvData::PVFieldPtr const &pvField);
    epics::pvData::MonitorPluginPtr getMonitorPlugin(size_t offset);
    PVRecordPtr pvRecord;
    epics::pvData::PVCopyPtr pvCopy;
    PVCopyMonitorRequesterPtr pvCopyMonitorRequester;
    epics::pvData::MonitorElementPtr monitorElement;
    bool isGroupPut;
    bool dataChanged;
    bool isMonitoring;
    epics::pvData::Mutex mutex;
    std::list<PVCopyMonitorFieldNodePtr> monitorFieldNodeList;
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
