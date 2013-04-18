/* pvRecord.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @data 2012.11.21
 */

#include <pv/pvDatabase.h>

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace std;

namespace epics { namespace pvDatabase {

PVRecordPtr PVRecord::create(
    String const &recordName,
    PVStructurePtr const & pvStructure)
{
    PVRecordPtr pvRecord(new PVRecord(recordName,pvStructure));
    if(!pvRecord->init()) {
        pvRecord->destroy();
        pvRecord.reset();
    }
    return pvRecord;
}


PVRecord::PVRecord(
    String const & recordName,
    PVStructurePtr const & pvStructure)
: recordName(recordName),
  pvStructure(pvStructure),
  convert(getConvert()),
  thelock(mutex),
  depthGroupPut(0),
  isDestroyed(false)
{
    thelock.unlock();
}

PVRecord::~PVRecord()
{
    destroy();
}

void PVRecord::initPVRecord()
{
    PVRecordStructurePtr parent;
    pvRecordStructure = PVRecordStructurePtr(
        new PVRecordStructure(pvStructure,parent,getPtrSelf()));
    pvRecordStructure->init();
    pvStructure->setRequester(getPtrSelf());
}

void PVRecord::destroy()
{
    lock();
    if(isDestroyed) {
        unlock();
        return;
    }
    isDestroyed = true;

    std::list<RequesterPtr>::iterator requesterIter;
    while(true) {
        requesterIter = requesterList.begin();
        if(requesterIter==requesterList.end()) break;
        requesterList.erase(requesterIter);
        unlock();
        (*requesterIter)->message("record destroyed",fatalErrorMessage);
        lock();
    }

    std::list<PVRecordClientPtr>::iterator clientIter;
    while(true) {
        clientIter = pvRecordClientList.begin();
        if(clientIter==pvRecordClientList.end()) break;
        pvRecordClientList.erase(clientIter);
        unlock();
        (*clientIter)->detach(getPtrSelf());
        lock();
    }
    pvListenerList.clear();
    pvRecordStructure->destroy();
    pvRecordStructure.reset();
    convert.reset();
    pvStructure.reset();
    unlock();
}

String PVRecord::getRecordName() {return recordName;}

PVRecordStructurePtr PVRecord::getPVRecordStructure() {return pvRecordStructure;}

PVStructurePtr PVRecord::getPVStructure() {return pvStructure;}

PVRecordFieldPtr PVRecord::findPVRecordField(PVFieldPtr const & pvField)
{
    return findPVRecordField(pvRecordStructure,pvField);
}

bool PVRecord::addRequester(epics::pvData::RequesterPtr const & requester)
{
    lock();
    if(isDestroyed) {
	 unlock();
	 return false;
    }
    std::list<RequesterPtr>::iterator iter;
    for (iter = requesterList.begin(); iter!=requesterList.end(); iter++ ) {
        if((*iter).get()==requester.get()) {
            unlock();
            return false;
        }
    }
    requesterList.push_back(requester);
    unlock();
    return true;
}

bool PVRecord::removeRequester(epics::pvData::RequesterPtr const & requester)
{
    lock();
    if(isDestroyed) {
	 unlock();
	 return false;
    }
    std::list<RequesterPtr>::iterator iter;
    for (iter = requesterList.begin(); iter!=requesterList.end(); iter++ ) {
        if((*iter).get()==requester.get()) {
            requesterList.erase(iter);
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

PVRecordFieldPtr PVRecord::findPVRecordField(
    PVRecordStructurePtr const & pvrs,
        PVFieldPtr const & pvField)
{
    size_t desiredOffset = pvField->getFieldOffset();
    PVFieldPtr pvf = pvrs->getPVField();
    size_t offset = pvf->getFieldOffset();
    if(offset==desiredOffset) return pvrs;
    PVRecordFieldPtrArrayPtr  pvrfpap = pvrs->getPVRecordFields();
    PVRecordFieldPtrArray::iterator iter;
    for (iter = pvrfpap.get()->begin(); iter!=pvrfpap.get()->end(); iter++ ) {
        PVRecordFieldPtr pvrf = *iter;
        pvf = pvrf->getPVField();
        offset = pvf->getFieldOffset();
        if(offset==desiredOffset) return pvrf;
        size_t nextOffset = pvf->getNextFieldOffset();
        if(nextOffset<=desiredOffset) continue;
        return findPVRecordField(
            static_pointer_cast<PVRecordStructure>(pvrf),
            pvField);
    }
    throw std::logic_error(
        recordName + " pvField "
        + pvField->getFieldName() + " not in PVRecord");
}

void PVRecord::lock() {thelock.lock();}

void PVRecord::unlock() {thelock.unlock();}

bool PVRecord::tryLock() {return thelock.tryLock();}

void PVRecord::lockOtherRecord(PVRecordPtr const & otherRecord)
{
    if(this<otherRecord.get()) {
        otherRecord->lock();
        return;
    }
    unlock();
    otherRecord->lock();
    lock();
}

bool PVRecord::addPVRecordClient(PVRecordClientPtr const & pvRecordClient)
{
    lock();
    if(isDestroyed) {
         unlock();
         return false;
    }
    std::list<PVRecordClientPtr>::iterator iter;
    for (iter = pvRecordClientList.begin();
    iter!=pvRecordClientList.end();
    iter++ )
    {
        if((*iter).get()==pvRecordClient.get()) {
            unlock();
            return false;
        }
    }
    pvRecordClientList.push_back(pvRecordClient);
    unlock();
    return true;
}

bool PVRecord::removePVRecordClient(PVRecordClientPtr const & pvRecordClient)
{
    lock();
    if(isDestroyed) {
         unlock();
         return false;
    }
    std::list<PVRecordClientPtr>::iterator iter;
    for (iter = pvRecordClientList.begin();
    iter!=pvRecordClientList.end();
    iter++ )
    {
        if((*iter).get()==pvRecordClient.get()) {
            pvRecordClientList.erase(iter);
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

void PVRecord::detachClients()
{
    lock();
    if(isDestroyed) {
         unlock();
         return;
    }
    std::list<PVRecordClientPtr>::iterator iter;
    for (iter = pvRecordClientList.begin();
    iter!=pvRecordClientList.end();
    iter++ )
    {
        (*iter)->detach(getPtrSelf());
    }
    pvRecordClientList.clear();
    unlock();
}

bool PVRecord::addListener(PVListenerPtr const & pvListener)
{
    lock();
    if(isDestroyed) {
         unlock();
         return false;
    }
    std::list<PVListenerPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ )
    {
        if((*iter).get()==pvListener.get()) {
            unlock();
            return false;
        }
    }
    pvListenerList.push_back(pvListener);
    unlock();
    return true;
}

bool PVRecord::removeListener(PVListenerPtr const & pvListener)
{
    lock();
    if(isDestroyed) {
         unlock();
         return false;
    }
    std::list<PVListenerPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ )
    {
        if((*iter).get()==pvListener.get()) {
            pvListenerList.erase(iter);
            pvRecordStructure->removeListener(pvListener);
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

void PVRecord::beginGroupPut()
{
   if(++depthGroupPut>1) return;
   std::list<PVListenerPtr>::iterator iter;
   for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++)
   {
       (*iter).get()->beginGroupPut(getPtrSelf());
   }
}

void PVRecord::endGroupPut()
{
   if(--depthGroupPut>0) return;
   std::list<PVListenerPtr>::iterator iter;
   for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++)
   {
       (*iter).get()->endGroupPut(getPtrSelf());
   }
}

void PVRecord::message(String const & message,MessageType messageType)
{
    if(isDestroyed) return;
    if(requesterList.size()==0 ) {
        String xxx(getMessageTypeName(messageType) + " " + message);
        printf("%s\n",xxx.c_str());
        return;
    }
    std::list<epics::pvData::RequesterPtr>::iterator iter;
    for(iter = requesterList.begin(); iter != requesterList.end(); ++iter) {
        (*iter)->message(message,messageType);
    }
}

void PVRecord::message(
    PVRecordFieldPtr const & pvRecordField,
    String const & message,
    MessageType messageType)
{
     this->message(pvRecordField->getFullName() + " " + message,messageType);
}

void PVRecord::toString(StringBuilder buf)
{
    toString(buf,0);
}

void PVRecord::toString(StringBuilder buf,int indentLevel)
{
    *buf += "\nrecord " + recordName + " ";
    pvRecordStructure->getPVStructure()->toString(buf, indentLevel);
}

PVRecordField::PVRecordField(
    PVFieldPtr const & pvField,
    PVRecordStructurePtr const &parent,
    PVRecordPtr const & pvRecord)
:  pvField(pvField),
   parent(parent),
   pvRecord(pvRecord)
{
}

void PVRecordField::init()
{
    fullFieldName = pvField->getFieldName();
    PVRecordStructurePtr pvParent = parent;
    while(pvParent.get()!= NULL) {
        String parentName = pvParent->getPVField()->getFieldName();
        if(parentName.size()>0) {
            fullFieldName = pvParent->getPVField()->getFieldName()
                + '.' + fullFieldName;
        }
        pvParent = pvParent->getParent();
    }
    if(fullFieldName.size()>0) {
        fullName = pvRecord->getRecordName() + '.' + fullFieldName;
    } else {
        fullName = pvRecord->getRecordName();
    }
    pvField->setPostHandler(getPtrSelf());
}

PVRecordField::~PVRecordField()
{
}

void PVRecordField::destroy()
{
    pvRecord.reset();
    parent.reset();
    pvField.reset();
    pvListenerList.clear();
}

PVRecordStructurePtr PVRecordField::getParent() {return parent;}

PVFieldPtr PVRecordField::getPVField() {return pvField;}

String PVRecordField::getFullFieldName() {return fullFieldName; }

String PVRecordField::getFullName() {return fullName; }

PVRecordPtr PVRecordField::getPVRecord() {return pvRecord;}

bool PVRecordField::addListener(PVListenerPtr const & pvListener)
{
    std::list<PVListenerPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ ) {
        if((*iter).get()==pvListener.get()) {
            return false;
        }
    }
    pvListenerList.push_back(pvListener);
    return true;
}

void PVRecordField::removeListener(PVListenerPtr const & pvListener)
{
    std::list<PVListenerPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ ) {
        if((*iter).get()==pvListener.get()) {
            pvListenerList.erase(iter);
            return;
        }
    }
}

void PVRecordField::postPut()
{
    callListener();
    std::list<PVListenerPtr>::iterator iter;
    PVRecordStructurePtr pvParent = getParent();
    while(pvParent.get()!=NULL) {
        std::list<PVListenerPtr> list = pvParent->pvListenerList;
        for (iter = list.begin(); iter!=list.end(); iter++ ) {
            (*iter)->dataPut(pvParent,getPtrSelf());
        }
        pvParent = pvParent->getParent();
    }
}

void PVRecordField::message(String const & message,MessageType messageType)
{
    pvRecord->message(getPtrSelf(),message,messageType);
}

void PVRecordField::callListener()
{
    std::list<PVListenerPtr>::iterator iter;
    for (iter = pvListenerList.begin(); iter!=pvListenerList.end(); iter++ ) {
        (*iter)->dataPut(getPtrSelf());
    }
}

PVRecordStructure::PVRecordStructure(
    PVStructurePtr const &pvStructure,
    PVRecordStructurePtr const &parent,
    PVRecordPtr const & pvRecord)
:
    PVRecordField(pvStructure,parent,pvRecord),
    pvStructure(pvStructure),
    pvRecordFields(new PVRecordFieldPtrArray)
{
}

PVRecordStructure::~PVRecordStructure()
{
}

void PVRecordStructure::destroy()
{
    PVRecordFieldPtrArray::iterator iter;
    PVRecordField::destroy();
    for(iter = pvRecordFields->begin() ; iter !=pvRecordFields->end(); iter++) {
        (*iter)->destroy();
    }
    pvRecordFields.reset();
    pvStructure.reset();
}

void PVRecordStructure::init()
{
    PVRecordField::init();
    const PVFieldPtrArray & pvFields = pvStructure->getPVFields();
    size_t numFields = pvFields.size();
    pvRecordFields->reserve( numFields);
    PVRecordStructurePtr self =
        static_pointer_cast<PVRecordStructure>(getPtrSelf());
    PVRecordPtr pvRecord = getPVRecord();
    for(size_t i=0; i<numFields; i++) {    
        PVFieldPtr pvField = pvFields[i];
        if(pvField->getField()->getType()==structure) {
             PVStructurePtr xxx = static_pointer_cast<PVStructure>(pvField);
             PVRecordStructurePtr pvRecordStructure(
                 new PVRecordStructure(xxx,self,pvRecord));
             pvRecordFields->push_back(pvRecordStructure);
             pvRecordStructure->init();
        } else {
             PVRecordFieldPtr pvRecordField(
                new PVRecordField(pvField,self,pvRecord));
             pvRecordFields->push_back(pvRecordField);
             pvRecordField->init();
        }
    }
}

PVRecordFieldPtrArrayPtr PVRecordStructure::getPVRecordFields()
{
    return pvRecordFields;
}

PVStructurePtr PVRecordStructure::getPVStructure() {return pvStructure;}

void PVRecordStructure::removeListener(PVListenerPtr const & pvListener)
{
    PVRecordField::removeListener(pvListener);
    size_t numFields = pvRecordFields->size();
    for(size_t i=0; i<numFields; i++) {
         PVRecordFieldPtr pvRecordField = (*pvRecordFields.get())[i];
         pvRecordField->removeListener(pvListener);
    }
}

void PVRecordStructure::postPut()
{
    PVRecordField::postPut();
    size_t numFields = pvRecordFields->size();
    for(size_t i=0; i<numFields; i++) {
         PVRecordFieldPtr pvRecordField = (*pvRecordFields.get())[i];
         pvRecordField->callListener();
    }
}

}}
