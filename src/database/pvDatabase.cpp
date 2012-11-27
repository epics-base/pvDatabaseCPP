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

PVRecord::PVRecord(
    String const & recordName,
    PVStructurePtr const & pvStructure)
: recordName(recordName),
  pvStructure(pvStructure)
{}

PVRecord::~PVRecord() {}

}}

