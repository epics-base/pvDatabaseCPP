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

#include <pv/channelProviderLocal.h>
#include <pv/pvCopyMonitor.h>


namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::size_t;
using std::cout;
using std::endl;

PVCopyMonitorPtr PVCopyMonitor::create(
    PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester,
    PVRecordPtr const &pvRecord,
    PVCopyPtr const & pvCopy)
{
    PVCopyMonitorPtr pvCopyMonitor( new PVCopyMonitor(
        pvRecord,pvCopy,pvCopyMonitorRequester));
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
 
void PVCopyMonitor::startMonitoring(
    BitSetPtr const  &changeBitSet,
    BitSetPtr const  &overrunBitSet)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::startMonitoring()" << endl;
    }
    Lock xx(mutex);
    if(isMonitoring) return;
    isMonitoring = true;
    this->changeBitSet = changeBitSet;
    this->overrunBitSet = overrunBitSet;
    isGroupPut = false;
    pvRecord->lock();
    try {
        pvRecord->addListener(getPtrSelf());
        pvCopy->traverseMaster(getPtrSelf());
        changeBitSet->clear();
        overrunBitSet->clear();
        changeBitSet->set(0);
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
    isMonitoring = false;
    pvRecord->removeListener(getPtrSelf());
}

void PVCopyMonitor::switchBitSets(
    BitSetPtr const &newChangeBitSet,
    BitSetPtr const &newOverrunBitSet)
{
    changeBitSet = newChangeBitSet;
    overrunBitSet = newOverrunBitSet;
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
    size_t offset = pvCopy->getCopyOffset(pvRecordField->getPVField());
    bool isSet = changeBitSet->get(offset);
    changeBitSet->set(offset);
    if(isSet) overrunBitSet->set(offset);
    if(!isGroupPut) pvCopyMonitorRequester->dataChanged();
    dataChanged = true;
}

void PVCopyMonitor::dataPut(
    PVRecordStructurePtr const & requested,
    PVRecordFieldPtr const & pvRecordField)
{
    size_t offsetCopyRequested = pvCopy->getCopyOffset(
        requested->getPVField());
    size_t offset = offsetCopyRequested
         + (pvRecordField->getPVField()->getFieldOffset()
             - requested->getPVField()->getFieldOffset());
#ifdef XXXXX
    CopyNodePtr node = findNode(headNode,requested);
    if(node.get()==NULL || node->isStructure) {
        throw std::logic_error("Logic error");
    }
    CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(node);
    size_t offset = recordNode->structureOffset
        + (pvRecordField->getPVField()->getFieldOffset()
             - recordNode->recordPVField->getPVField()->getFieldOffset());
#endif
    bool isSet = changeBitSet->get(offset);
    changeBitSet->set(offset);
    if(isSet) overrunBitSet->set(offset);
    if(!isGroupPut) pvCopyMonitorRequester->dataChanged();
    dataChanged = true;
}

void PVCopyMonitor::beginGroupPut(PVRecordPtr const & pvRecord)
{
    isGroupPut = true;
    dataChanged = false;
}

void PVCopyMonitor::endGroupPut(PVRecordPtr const & pvRecord)
{
    isGroupPut = false;
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
