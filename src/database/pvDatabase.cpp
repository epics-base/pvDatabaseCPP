/* pvDatabase.cpp */
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

PVDatabasePtr PVDatabase::getMaster()
{
    static PVDatabasePtr master;
    static Mutex mutex;
    Lock xx(mutex);
    if(master.get()==NULL) {
        master = PVDatabasePtr(new PVDatabase());
    }
    return master;
}

PVDatabase::PVDatabase()
: thelock(mutex),
  isDestroyed(false)
{
    thelock.unlock();
}

PVDatabase::~PVDatabase()
{
}

void PVDatabase::destroy()
{
    lock();
    if(isDestroyed) {
        unlock();
        return;
    }
    isDestroyed = true;
    PVRecordMap::iterator iter;
    while(true) {
        iter = recordMap.begin();
        if(iter==recordMap.end()) break;
        PVRecordPtr pvRecord = (*iter).second;
        recordMap.erase(iter);
        unlock();
        if(pvRecord.get()!=NULL) pvRecord->destroy();
        lock();
    }
}

void PVDatabase::lock() {
    thelock.lock();
}

void PVDatabase::unlock() {
    thelock.unlock();
}

PVRecordPtr PVDatabase::findRecord(String const& recordName)
{
    lock_guard();
    PVRecordPtr xxx;
    if(isDestroyed) return xxx;
    PVRecordMap::iterator iter = recordMap.find(recordName);
    if(iter!=recordMap.end()) {
         return (*iter).second;
    }
    return xxx;
}

PVStringArrayPtr PVDatabase::getRecordNames()
{
    lock_guard();
    PVStringArrayPtr pvStringArray = static_pointer_cast<PVStringArray>
        (getPVDataCreate()->createPVScalarArray(pvString));
    size_t len = recordMap.size();
    std::vector<String> names(len);
    PVRecordMap::iterator iter;
    size_t i = 0;
    for(iter = recordMap.begin(); iter!=recordMap.end(); ++iter) {
        names[i++] = (*iter).first;
    }
    pvStringArray->put(0,len,names,0);
    return pvStringArray;
}

bool PVDatabase::addRecord(PVRecordPtr const & record)
{
    lock_guard();
    if(isDestroyed) return false;
    String recordName = record->getRecordName();
    PVRecordMap::iterator iter = recordMap.find(recordName);
    if(iter!=recordMap.end()) {
         return false;
    }
    recordMap.insert(PVRecordMap::value_type(recordName,record));
    return true;
}

bool PVDatabase::removeRecord(PVRecordPtr const & record)
{
    lock();
    if(isDestroyed) return false;
    String recordName = record->getRecordName();
    PVRecordMap::iterator iter = recordMap.find(recordName);
    if(iter!=recordMap.end())  {
        PVRecordPtr pvRecord = (*iter).second;
        recordMap.erase(iter);
        unlock();
        if(pvRecord.get()!=NULL) pvRecord->destroy();
        return true;
    }
    unlock();
    return false;
}

String PVDatabase::getRequesterName()
{
    static String name("masterDatabase");
    return name;
}

void PVDatabase::message(String const & message,MessageType messageType)
{
}

}}
