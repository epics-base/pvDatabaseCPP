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
#include <pv/powerSupplyRecordTest.h>


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

static PowerSupplyRecordTestPtr createPowerSupply(String const & recordName)
{
    FieldCreatePtr fieldCreate = getFieldCreate();
    StandardFieldPtr standardField = getStandardField();
    PVDataCreatePtr pvDataCreate = getPVDataCreate();
    size_t nfields = 5;
    StringArray names;
    names.reserve(nfields);
    FieldConstPtrArray powerSupply;
    powerSupply.reserve(nfields);
    names.push_back("alarm");
    powerSupply.push_back(standardField->alarm());
    names.push_back("timeStamp");
    powerSupply.push_back(standardField->timeStamp());
    String properties("alarm,display");
    names.push_back("voltage");
    powerSupply.push_back(standardField->scalar(pvDouble,properties));
    names.push_back("power");
    powerSupply.push_back(standardField->scalar(pvDouble,properties));
    names.push_back("current");
    powerSupply.push_back(standardField->scalar(pvDouble,properties));
    return PowerSupplyRecordTest::create(recordName,
        pvDataCreate->createPVStructure(
            fieldCreate->createStructure(names,powerSupply)));
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

    pvRecord->lock_guard();
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
    pvCopy->initCopy(pvStructureCopy, bitSet, true);
    cout << "after initCopy pvValueCopy " << convert->toDouble(pvValueCopy);
    cout << endl;
    convert->fromDouble(pvValueRecord,.06);
    pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet,true);
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
    pvCopy->updateCopyFromBitSet(pvStructureCopy,bitSet,true);
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
    pvCopy->updateRecord(pvStructureCopy,bitSet,true);
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
ConvertPtr convert = getConvert();
    size_t n = 5;
DoubleArray values(n);
//    shared_vector<double> values(n);

    pvRecord->lock_guard();
    cout << endl;
    pvStructureRecord = pvRecord->getPVRecordStructure()->getPVStructure();
    pvValueRecord = pvStructureRecord->getScalarArrayField(valueNameRecord,scalarType);
    for(size_t i=0; i<n; i++) values[i] = i;
convert->fromDoubleArray(pvValueRecord,0,n,get(values),0);
//     pvValueRecord->PVScalarArray::putFrom<pvDouble>(values);
    StructureConstPtr structure = pvCopy->getStructure();
    builder.clear(); structure->toString(&builder);
    cout << "structure from copy" << endl << builder << endl;
    pvStructureCopy = pvCopy->createPVStructure();
    pvValueCopy = pvStructureCopy->getScalarArrayField(valueNameCopy,scalarType);
    bitSet = BitSetPtr(new BitSet(pvStructureCopy->getNumberFields()));
    pvCopy->initCopy(pvStructureCopy, bitSet, true);
    builder.clear(); pvValueCopy->toString(&builder);
    cout << "after initCopy pvValueCopy " << builder << endl;
    cout << endl;
    for(size_t i=0; i<n; i++) values[i] = i + .06;
convert->fromDoubleArray(pvValueRecord,0,n,get(values),0);
//    pvValueRecord->PVScalarArray::putFrom<pvDouble>(values);
    pvCopy->updateCopySetBitSet(pvStructureCopy,bitSet,true);
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
    for(size_t i=0; i<n; i++) values[i] = i + 1.0;
convert->fromDoubleArray(pvValueRecord,0,n,get(values),0);
//    pvValueRecord->PVScalarArray::putFrom<pvDouble>(values);
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
    pvCopy->updateCopyFromBitSet(pvStructureCopy,bitSet,true);
    cout << "after updateCopyFromBitSet";
    builder.clear(); pvValueRecord->toString(&builder);
    cout << " recordValue " << builder << endl;
    builder.clear(); pvValueCopy->toString(&builder);
    cout << " copyValue " << builder << endl;
    builder.clear();
    bitSet->toString(&builder);
    cout << " bitSet " << builder;
    cout << endl;
    for(size_t i=0; i<n; i++) values[i] = i + 2.0;
convert->fromDoubleArray(pvValueRecord,0,n,get(values),0);
//    pvValueRecord->PVScalarArray::putFrom<pvDouble>(values);
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
    pvCopy->updateRecord(pvStructureCopy,bitSet,true);
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
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl;
    cout << "pvRequest" << endl << builder;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,value";
    valueNameRecord = "value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
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

    pvRecord = createScalarArray("doubleArrayRecord",pvDouble,"alarm,timeStamp");
    valueNameRecord = request = "value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl;
    cout << "pvRequest" << endl << builder;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(pvDouble,valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalarArray(pvDouble,valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,value";
    valueNameRecord = "value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
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
    PowerSupplyRecordTestPtr pvRecord;
    String request;
    PVStructurePtr pvRequest;
    PVRecordFieldPtr pvRecordField;
    PVCopyPtr pvCopy;
    String builder;
    String valueNameRecord;
    String valueNameCopy;

    pvRecord = createPowerSupply("powerSupply");
    valueNameRecord = request = "power.value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl;
    cout << "pvRequest" << endl << builder;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "";
    valueNameRecord = "power.value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "power.value";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,voltage.value,power.value,current.value";
    valueNameRecord = "power.value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
    builder.clear(); pvRequest->toString(&builder);
    cout << "request " << request << endl << "pvRequest" << endl << builder << endl;
    pvCopy = PVCopy::create(pvRecord,pvRequest,"");
    valueNameCopy = "power";
    testPVScalar(valueNameRecord,valueNameCopy,pvRecord,pvCopy);
    request = "alarm,timeStamp,voltage{value,alarm},power{value,alarm,display},current.value";
    valueNameRecord = "power.value";
    pvRequest = getCreateRequest()->createRequest(request,requester);
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

