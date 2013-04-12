/* exampleCounter.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.02
 */
#ifndef EXAMPLECOUNTER_H
#define EXAMPLECOUNTER_H

#include <pv/pvDatabase.h>
#include <pv/standardPVField.h>

namespace epics { namespace pvDatabase { 


class ExampleCounter;
typedef std::tr1::shared_ptr<ExampleCounter> ExampleCounterPtr;

class ExampleCounter :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(ExampleCounter);
    static ExampleCounterPtr create(
        epics::pvData::String const & recordName);
    virtual ~ExampleCounter(){}
    virtual bool init();
    virtual void process();
private:
    ExampleCounter(epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    epics::pvData::PVLongPtr pvValue;
};

ExampleCounterPtr ExampleCounter::create(
    epics::pvData::String const & recordName)
{
    epics::pvData::PVStructurePtr pvStructure =
       epics::pvData::getStandardPVField()->scalar(epics::pvData::pvLong,"timeStamp");
    ExampleCounterPtr pvRecord(
        new ExampleCounter(recordName,pvStructure));
    if(!pvRecord->init()) pvRecord.reset();
    return pvRecord;
}

ExampleCounter::ExampleCounter(
    epics::pvData::String const & recordName,
    epics::pvData::PVStructurePtr const & pvStructure)
: PVRecord(recordName,pvStructure)
{
}

bool ExampleCounter::init()
{
    
    initPVRecord();
    epics::pvData::PVFieldPtr pvField;
    pvValue = getPVStructure()->getLongField("value");
    if(pvValue.get()==NULL) return false;
    return true;
}

void ExampleCounter::process()
{
    pvValue->put(pvValue->get() + 1.0);
}

}}

#endif  /* EXAMPLECOUNTER_H */
