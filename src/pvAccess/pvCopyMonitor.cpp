/* pvCopyMonitor.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>

#include <pv/thread.h>

#define epicsExportSharedSymbols

#include <pv/channelProviderLocal.h>
#include <pv/pvCopyMonitor.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::size_t;
using std::cout;
using std::endl;

struct PVCopyMonitorFieldNode
{
    MonitorPluginPtr monitorPlugin;
    size_t offset;   // in pvCopy
};

PVCopyMonitorPtr PVCopyMonitor::create(
    PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester,
    PVRecordPtr const &pvRecord,
    PVCopyPtr const & pvCopy)
{
    PVCopyMonitorPtr pvCopyMonitor( new PVCopyMonitor(
        pvRecord,pvCopy,pvCopyMonitorRequester));
    pvCopyMonitor->init(pvRecord->getPVRecordStructure()->getPVStructure());
    return pvCopyMonitor;
}


PVCopyMonitor::PVCopyMonitor(
    PVRecordPtr const &pvRecord,
    PVCopyPtr const &pvCopy,
    PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester
)
: pvRecord(pvRecord),
  pvCopy(pvCopy),
  pvCopyMonitorRequester(pvCopyMonitorRequester),
  isGroupPut(false),
  dataChanged(false),
  isMonitoring(false)
{
}

PVCopyMonitor::~PVCopyMonitor()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "~PVCopyMonitor" << endl;
    }
}

// pvField is in top level structure of PVRecord.
void PVCopyMonitor::init(PVFieldPtr const &pvField)
{
    size_t offset = pvCopy->getCopyOffset(pvField);
    if(offset==String::npos) return;
    PVStructurePtr pvOptions = pvCopy->getOptions(offset);
    if(pvOptions!=NULL) {
        PVStringPtr pvName = pvOptions->getSubField<PVString>("plugin");
        if(pvName!=NULL) {
            String pluginName = pvName->get();
            MonitorPluginManagerPtr manager = MonitorPluginManager::get();
            MonitorPluginCreatorPtr pluginCreator = manager->findPlugin(pluginName);
            if(pluginCreator!=NULL) {
                StructureConstPtr top = pvCopy->getStructure();
                FieldConstPtr field = pvField->getField();
                MonitorPluginPtr monitorPlugin = pluginCreator->create(field,top,pvOptions);
                if(monitorPlugin!=NULL) {
                     PVCopyMonitorFieldNodePtr fieldNode(new PVCopyMonitorFieldNode());
                     fieldNode->monitorPlugin = monitorPlugin;
                     fieldNode->offset = offset;
                     monitorFieldNodeList.push_back(fieldNode);
                }
            }
        }
    }
    if(pvField->getField()->getType()!=structure) return;
    PVStructurePtr pv = static_pointer_cast<PVStructure>(pvField);
    const PVFieldPtrArray &pvFields = pv->getPVFields();
    for(size_t i=0; i<pvFields.size(); ++i ) init(pvFields[i]);
}

MonitorPluginPtr PVCopyMonitor::getMonitorPlugin(size_t offset)
{
     std::list<PVCopyMonitorFieldNodePtr>::iterator iter;
     for (iter = monitorFieldNodeList.begin();iter!=monitorFieldNodeList.end();++iter)
     {
         if((*iter)->offset==offset) return (*iter)->monitorPlugin;
     }
     return MonitorPluginPtr();
}

void PVCopyMonitor::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::destroy()" << endl;
    }
    stopMonitoring();
    pvCopyMonitorRequester.reset();
    pvCopy.reset();
}
 
void PVCopyMonitor::startMonitoring()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::startMonitoring()" << endl;
    }
    Lock xx(mutex);
    if(isMonitoring) return;
    isMonitoring = true;
    isGroupPut = false;
    std::list<PVCopyMonitorFieldNodePtr>::iterator iter;
    for (iter = monitorFieldNodeList.begin();iter!=monitorFieldNodeList.end();++iter)
    {
       (*iter)->monitorPlugin->startMonitoring();
    }
    pvRecord->lock();
    try {
        pvRecord->addListener(getPtrSelf());
        pvCopy->traverseMaster(getPtrSelf());
        monitorElement->changedBitSet->clear();
        monitorElement->overrunBitSet->clear();
        monitorElement->changedBitSet->set(0);
        pvCopyMonitorRequester->dataChanged();
        pvRecord->unlock();
    } catch(...) {
        pvRecord->unlock();
    }
}

void PVCopyMonitor::nextMasterPVField(epics::pvData::PVFieldPtr const &pvField)
{
    pvRecord->findPVRecordField(pvField)->addListener(getPtrSelf());
}

void PVCopyMonitor::stopMonitoring()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::stopMonitoring()" << endl;
    }
    Lock xx(mutex);
    if(!isMonitoring) return;
    std::list<PVCopyMonitorFieldNodePtr>::iterator iter;
    for (iter = monitorFieldNodeList.begin();iter!=monitorFieldNodeList.end();++iter)
    {
       (*iter)->monitorPlugin->stopMonitoring();
    }
    isMonitoring = false;
    pvRecord->removeListener(getPtrSelf());
}


void PVCopyMonitor::setMonitorElement(MonitorElementPtr const &xxx)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::setMonitorElement()" << endl;
    }
    monitorElement = xxx;
}

void PVCopyMonitor::monitorDone(MonitorElementPtr const &monitorElement)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::monitorDone()" << endl;
    }
    std::list<PVCopyMonitorFieldNodePtr>::iterator iter;
    for (iter = monitorFieldNodeList.begin();iter!=monitorFieldNodeList.end();++iter)
    {
        PVCopyMonitorFieldNodePtr fieldNode = *iter;
        MonitorPluginPtr monitorPlugin = fieldNode->monitorPlugin;
        monitorPlugin->monitorDone(monitorElement);
    }
}

void PVCopyMonitor::detach(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::detach()" << endl;
    }
}

void PVCopyMonitor::dataPut(PVRecordFieldPtr const & pvRecordField)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::dataPut(pvRecordField)" << endl;
    }
    size_t offset = pvCopy->getCopyOffset(pvRecordField->getPVField());
    BitSetPtr const &changedBitSet = monitorElement->changedBitSet;
    BitSetPtr const &overrunBitSet = monitorElement->overrunBitSet;
    bool isSet = changedBitSet->get(offset);
    changedBitSet->set(offset);
    if(isSet) overrunBitSet->set(offset);
    MonitorPluginPtr  monitorPlugin = getMonitorPlugin(offset);
    bool causeMonitor = true;
    if(monitorPlugin!=NULL) {
        causeMonitor = monitorPlugin->causeMonitor(
           pvRecordField->getPVField(),
           pvRecord->getPVRecordStructure()->getPVStructure(),
           monitorElement);
    }
    if(causeMonitor) {
        if(!isGroupPut) pvCopyMonitorRequester->dataChanged();
        dataChanged = true;
    }
}

void PVCopyMonitor::dataPut(
    PVRecordStructurePtr const & requested,
    PVRecordFieldPtr const & pvRecordField)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::dataPut(requested,pvRecordField)" << endl;
    }
    BitSetPtr const &changedBitSet = monitorElement->changedBitSet;
    BitSetPtr const &overrunBitSet = monitorElement->overrunBitSet;
    size_t offsetCopyRequested = pvCopy->getCopyOffset(
        requested->getPVField());
    size_t offset = offsetCopyRequested
         + (pvRecordField->getPVField()->getFieldOffset()
             - requested->getPVField()->getFieldOffset());
    bool isSet = changedBitSet->get(offset);
    changedBitSet->set(offset);
    if(isSet) overrunBitSet->set(offset);
    MonitorPluginPtr  monitorPlugin = getMonitorPlugin(offsetCopyRequested);
    bool causeMonitor = true;
    if(monitorPlugin!=NULL) {
        causeMonitor = monitorPlugin->causeMonitor(
           requested->getPVField(),
           pvRecord->getPVRecordStructure()->getPVStructure(),
           monitorElement);
    }
    if(causeMonitor) {
        if(!isGroupPut) pvCopyMonitorRequester->dataChanged();
        dataChanged = true;
    }
}

void PVCopyMonitor::beginGroupPut(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::beginGroupPut()" << endl;
    }
    isGroupPut = true;
    dataChanged = false;
    std::list<PVCopyMonitorFieldNodePtr>::iterator iter;
    for (iter = monitorFieldNodeList.begin();
    iter!=monitorFieldNodeList.end();
    ++iter)
    {
       (*iter)->monitorPlugin->beginGroupPut();
    }
}

void PVCopyMonitor::endGroupPut(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::endGroupPut() dataChanged " << dataChanged << endl;
    }
    isGroupPut = false;
    std::list<PVCopyMonitorFieldNodePtr>::iterator iter;
    for (iter = monitorFieldNodeList.begin();iter!=monitorFieldNodeList.end();++iter)
    {
       (*iter)->monitorPlugin->endGroupPut();
    }
    if(dataChanged) {
         dataChanged = false;
         pvCopyMonitorRequester->dataChanged();
    }
}

void PVCopyMonitor::unlisten(PVRecordPtr const & pvRecord)
{
    pvCopyMonitorRequester->unlisten();
}



}}
