/* monitorAlgorithm.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#ifndef MONITORALGORITHM_H
#define MONITORALGORITHM_H

#include <shareLib.h>

#include <pv/pvCopy.h>

namespace epics { namespace pvDatabase { 

class MonitorAlgorithm;
typedef std::tr1::shared_ptr<MonitorAlgorithm> MonitorAlgorithmPtr;
class MonitorAlgorithmCreate;
typedef std::tr1::shared_ptr<MonitorAlgorithmCreate> MonitorAlgorithmCreatePtr;

class epicsShareClass MonitorAlgorithm 
{
public:
    POINTER_DEFINITIONS(MonitorAlgorithm);
    virtual ~MonitorAlgorithm(){}
    virtual epics::pvData::String getAlgorithmName(){return algorithmName;}
    virtual bool causeMonitor() = 0;
    virtual void monitorIssued() = 0;
protected:
    MonitorAlgorithm(epics::pvData::String algorithmName)
    : algorithmName(algorithmName)
    {}
    epics::pvData::String algorithmName;
};

class epicsShareClass MonitorAlgorithmCreate 
{
public:
    POINTER_DEFINITIONS(MonitorAlgorithmCreate);
    virtual ~MonitorAlgorithmCreate(){}
    virtual epics::pvData::String getAlgorithmName(){return algorithmName;}
    virtual MonitorAlgorithmPtr create(
        PVRecordPtr const &pvRecord,
        epics::pvData::MonitorRequester::shared_pointer const &requester,
        PVRecordFieldPtr const &fromPVRecord,
        epics::pvData::PVStructurePtr const &pvOptions) = 0;
protected:
    MonitorAlgorithmCreate(epics::pvData::String algorithmName)
    : algorithmName(algorithmName)
    {}
    epics::pvData::String algorithmName;
};

}}

#endif  /* MONITORALGORITHM_H */
