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

PVDatabase::~PVDatabase() {}

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

PVDatabase::PVDatabase() {}

PVRecordPtr PVDatabase::findRecord(String const& recordName)
{
     PVRecordMap::iterator iter = recordMap.find(recordName);
     if(iter!=recordMap.end()) {
         return (*iter).second;
     }
     PVRecordPtr xxx;
     return xxx;
}

bool PVDatabase::addRecord(PVRecordPtr const & record)
{
     String recordName = record->getRecordName();
     PVRecordMap::iterator iter = recordMap.find(recordName);
     if(iter!=recordMap.end()) return false;
     recordMap.insert(PVRecordMap::value_type(recordName,record));
     return true;
}

bool PVDatabase::removeRecord(PVRecordPtr const & record)
{
     String recordName = record->getRecordName();
     PVRecordMap::iterator iter = recordMap.find(recordName);
     if(iter!=recordMap.end())  {
         recordMap.erase(iter);
         return true;
     }
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
