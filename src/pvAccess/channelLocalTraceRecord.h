/* channelLocalTraceRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */
#ifndef CHANNELLOCALTRACERECORD_H
#define CHANNELLOCALTRACERECORD_H

#include <pv/channelProviderLocal.h>

namespace epics { namespace pvDatabase { 


class ChannelLocalTraceRecord;
typedef std::tr1::shared_ptr<ChannelLocalTraceRecord> ChannelLocalTraceRecordPtr;

class ChannelLocalTraceRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(ChannelLocalTraceRecord);
    static ChannelLocalTraceRecordPtr create(
        ChannelLocalTracePtr const &channelLocalTrace,
        epics::pvData::String const & recordName);
    virtual ~ChannelLocalTraceRecord();
    virtual void destroy();
    virtual bool init();
    virtual void process();
private:
    ChannelLocalTraceRecord(
        ChannelLocalTracePtr const &channelLocalTrace,
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    ChannelLocalTracePtr channelLocalTrace;
    epics::pvData::PVIntPtr pvValue;
    bool isDestroyed;
};

}}

#endif  /* CHANNELLOCALTRACERECORD_H */
