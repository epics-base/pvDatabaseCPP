/*testPVCopyMain.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 */

/* Author: Marty Kraimer */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstdio>
#include <memory>
#include <iostream>

#include <epicsStdio.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsThread.h>

#include <pv/standardField.h>
#include <pv/standardPVField.h>
#include <pv/channelProviderLocal.h>
#include "powerSupply.h"


using namespace std;
using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;


class MyRequester;
typedef std::tr1::shared_ptr<MyRequester> MyRequesterPtr;

class MyRequester : public Requester {
public:
    POINTER_DEFINITIONS(MyRequester);
    MyRequester(String const &requesterName)
    : requesterName(requesterName)
    {}
    virtual ~MyRequester() {}
    virtual String getRequesterName() { return requesterName;}
    virtual void message(String const & message,MessageType messageType)
    {
         cout << message << endl;
    }
private:
    String requesterName;
};

static PVRecordPtr createScalar(
    String const & recordName,
    ScalarType scalarType,
    String const & properties)
{
    PVStructurePtr pvStructure = getStandardPVField()->scalar(scalarType,properties);
    return PVRecord::create(recordName,pvStructure);
}

static PVRecordPtr createScalarArray(
    String const & recordName,
    ScalarType scalarType,
    String const & properties)
{
    PVStructurePtr pvStructure = getStandardPVField()->scalarArray(scalarType,properties);
    return PVRecord::create(recordName,pvStructure);
}

static void testPVScalar(
    String const & valueNameRecord,
    String const & valueNameCopy,
    PVRecordPtr const & pvRecord,
    PVCopyPtr const & pvCopy)
{
    PVRecordFieldPtr pvRecordField;
    PVStructurePtr pvStructureRecord;
    PVStructurePtr pvStructureCopy;
    PVFieldPtr pvField;
    PVScalarPtr pvValueRecord;
    PVScalarPtr pvValueCopy;
    BitSetPtr bitSet;
    String builder;
    size_t offset;
    ConvertPtr convert = getConvert();

    cout << endl;
    pvStructureRecord = pvRecord->getPVRecordStructure()->getPVStructure();
    pvField = pvStructureRecord->getSubField(valueNameRecord);
    pvValueRecord = static_pointer_cast<PVScalar>(pvField);
    convert->fromDouble(pvValueRecord,.04);
    StructureConstPtr structure = pvCopy->getStructure();
    builder.clear(); structure->toString(&builder);
    cout << "structure from copy" << endl << builder << endl;
    pvStructureCopy = pvCopy->createPVStructure();
    pvField = pvStructureCopy->getSubField(valueNameCopy);
    pvValueCopy = static_pointer_cast<PVScalar>(pvField);
    bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvCopy->initCopy(pvStructureCopy, bitSet);
    cout << "after initCopy pvValueCopy " << convert->toDouble(pvValueCopy);
    cout << endl;
    convert->fromDouble(pvValueRecord,.06);
    pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    cout << "after put(.06) pvValueCopy " << convert->toDouble(pvValueCopy);
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
    pvRecordField = pvRecord->findPVRecordField(pvValueRecord);
    offset = pvCopy->getCopyOffset(pvRecordField);
    cout << "getCopyOffset() " << offset;
    cout << " pvValueCopy->getOffset() " << pvValueCopy->getFieldOffset();
    cout << " pvValueRecord->getOffset() " << pvValueRecord->getFieldOffset();
    cout << " bitSet " << builder;
    cout << endl;
    bitSet->clear();
    convert->fromDouble(pvValueRecord,1.0);
    builder.clear();
    bitSet->toString(&builder);
    cout << "before updateCopyFromBitSet";
    cout << " recordValue " << convert->toDouble(pvValueRecord);
    cout << " copyValue " << convert->toDouble(pvValueCopy);
    cout << " bitSet " << builder;
    cout << endl;
    bitSet->set(0);
    pvCopy->updateCopyFromBitSet(pvStructureCopy,bitSet);
    cout << "after updateCopyFromBitSet";
    cout << " recordValue " << convert->toDouble(pvValueRecord);
    cout << " copyValue " << convert->toDouble(pvValueCopy);
    cout << " bitSet " << builder;
    cout << endl;
    convert->fromDouble(pvValueCopy,2.0);
    bitSet->set(0);
    cout << "before updateRecord";
    cout << " recordValue " << convert->toDouble(pvValueRecord);
    cout << " copyValue " << convert->toDouble(pvValueCopy);
    cout << " bitSet " << builder;
    cout << endl;
    pvCopy->updateRecord(pvStructureCopy,bitSet);
    cout << "after updateRecord";
    cout << " recordValue " << convert->toDouble(pvValueRecord);
    cout << " copyValue " << convert->toDouble(pvValueCopy);
    cout << " bitSet " << builder;
    cout << endl;
}

static void testPVScalarArray(
    ScalarType scalarType,
    String const & valueNameRecord,
    String const & valueNameCopy,
    PVRecordPtr const & pvRecord,
    PVCopyPtr const & pvCopy)
{
    PVRecordFieldPtr pvRecordField;
    PVStructurePtr pvStructureRecord;
    PVStructurePtr pvStructureCopy;
    PVScalarArrayPtr pvValueRecord;
    PVScalarArrayPtr pvValueCopy;
    BitSetPtr bitSet;
    String builder;
    size_t offset;
    size_t n = 5;
    shared_vector<double> values(n);
    cout << endl;
    pvStructureRecord = pvRecord->getPVRecordStructure()->getPVStructure();
    pvValueRecord = pvStructureRecord->getScalarArrayField(valueNameRecord,scalarType);
    for(size_t i=0; i<n; i++) values[i] = i;
    const shared_vector<const double> xxx(freeze(values));
    pvValueRecord->putFrom(xxx);
    StructureConstPtr structure = pvCopy->getStructure();
    builder.clear(); structure->toString(&builder);
    cout << "structure from copy" << endl << builder << endl;
    pvStructureCopy = pvCopy->createPVStructure();
    pvValueCopy = pvStructureCopy->getScalarArrayField(valueNameCopy,scalarType);
    bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvCopy->initCopy(pvStructureCopy, bitSet);
    builder.clear(); pvValueCopy->toString(&builder);
    cout << "after initCopy pvValueCopy " << builder << endl;
    cout << endl;
    values.resize(n);
    for(size_t i=0; i<n; i++) values[i] = i + .06;
    const shared_vector<const double> yyy(freeze(values));
    pvValueRecord->putFrom(yyy);
    pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet);
    builder.clear(); pvValueCopy->toString(&builder);
    cout << "after put(i+ .06) pvValueCopy " << builder << endl;
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
    pvRecordField = pvRecord->findPVRecordField(pvValueRecord);
    offset = pvCopy->getCopyOffset(pvRecordField);
    cout << "getCopyOffset() " << offset;
    cout << " pvValueCopy->getOffset() " << pvValueCopy->getFieldOffset();
    cout << " pvValueRecord->getOffset() " << pvValueRecord->getFieldOffset();
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
    bitSet->clear();
    values.resize(n);
    for(size_t i=0; i<n; i++) values[i] = i + 1.0;
    const shared_vector<const double> zzz(freeze(values));
    pvValueRecord->putFrom(zzz);
    builder.clear();
    bitSet->toString(&builder);
    cout << "before updateCopyFromBitSet";
    builder.clear(); pvValueRecord->toString(&builder);
    cout << " recordValue " << builder << endl;
    builder.clear(); pvValueCopy->toString(&builder);
    cout << " copyValue " << builder << endl;
    cout << " bitSet " << builder;
    builder.clear();
    bitSet->toString(&builder);
    cout << endl;
    bitSet->set(0);
    pvCopy->updateCopyFromBitSet(pvStructureCopy,bitSet);
    cout << "after updateCopyFromBitSet";
    builder.clear(); pvValueRecord->toString(&builder);
    cout << " recordValue " << builder << endl;
    builder.clear(); pvValueCopy->toString(&builder);
    cout << " copyValue " << builder << endl;
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
    values.resize(n);
    for(size_t i=0; i<n; i++) values[i] = i + 2.0;
    const shared_vector<const double> ttt(freeze(values));
    pvValueRecord->putFrom(ttt);
    bitSet->set(0);
    cout << "before updateRecord";
    builder.clear(); pvValueRecord->toString(&builder);
    cout << " recordValue " << builder << endl;
    builder.clear(); pvValueCopy->toString(&builder);
    cout << " copyValue " << builder << endl;
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
    pvCopy->updateRecord(pvStructureCopy,bitSet);
    cout << "after updateRecord";
    builder.clear(); pvValueRecord->toString(&builder);
    cout << " recordValue " << builder << endl;
    builder.clear(); pvValueCopy->toString(&builder);
    cout << " copyValue " << builder << endl;
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
}
    
static void scalarTest()
{
    cout << endl << endl << "****scalarTest****" << endl;
    RequesterPtr requester(new MyRequester("exampleTest"));
    PVRecordPtr pvRecord;
    String request;
    PVStructurePtr pvRequest;
    PVRecordFieldPtr pvRecordField;
    PVCopyPtr pvCopy;
    String builder;
    String valueNameRecord;
    String valueNameCopy;

    pvRecord = createScalar("doubleRecord",pvDouble,"alarm,timeStamp,display");
    valueNameRecord = request = "value";
    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl;
    cout << "pvRequest" << endl << builder;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,value";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    pvRecord->destroy();
}

static void arrayTest()
{
    cout << endl << endl << "****arrayTest****" << endl;
    RequesterPtr requester(new MyRequester("exampleTest"));
    PVRecordPtr pvRecord;
    String request;
    PVStructurePtr pvRequest;
    PVRecordFieldPtr pvRecordField;
    PVCopyPtr pvCopy;
    String builder;
    String valueNameRecord;
    String valueNameCopy;

    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    pvRecord = createScalarArray("doubleArrayRecord",pvDouble,"alarm,timeStamp");
    valueNameRecord = request = "value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl;
    cout << "pvRequest" << endl << builder;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(pvDouble,valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(pvDouble,valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,value";
    valueNameRecord = "value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(pvDouble,valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    pvRecord->destroy();
}

static void powerSupplyTest()
{
    cout << endl << endl << "****powerSupplyTest****" << endl;
    RequesterPtr requester(new MyRequester("exampleTest"));
    PowerSupplyPtr pvRecord;
    String request;
    PVStructurePtr pvRequest;
    PVRecordFieldPtr pvRecordField;
    PVCopyPtr pvCopy;
    String builder;
    String valueNameRecord;
    String valueNameCopy;

    CreateRequest::shared_pointer createRequest = CreateRequest::create();
    PVStructurePtr pv = createPowerSupply();
    pvRecord = PowerSupply::create("powerSupply",pv);
    valueNameRecord = request = "power.value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl;
    cout << "pvRequest" << endl << builder;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "power.value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,voltage.value,power.value,current.value";
    valueNameRecord = "power.value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "power";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,voltage{value,alarm},power{value,alarm,display},current.value";
    valueNameRecord = "power.value";
    pvRequest = createRequest->createRequest(request);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    pvRecord->destroy();
}

int main(int argc,char *argv[])
{
    scalarTest();
    arrayTest();
    powerSupplyTest();
    return 0;
}

