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

namespace epics { namespace pvDatabase {

PVDatabase::~PVDatabase() {}

PVDatabasePtr PVDatabase::getMaster()
{
     PVDatabasePtr xxx;
     return xxx;
}

PVRecordPtr PVDatabase::findRecord(String const& recordName)
{
     PVRecordPtr xxx;
     return xxx;
}

bool PVDatabase::addRecord(PVRecordPtr const & record)
{
    return false;
}

bool PVDatabase::removeRecord(PVRecordPtr const & record)
{
    return false;
}

String PVDatabase::getRequesterName()
{
    String xxx;
    return xxx;
}

void PVDatabase::message(String const & message,MessageType messageType)
{
}

}}
