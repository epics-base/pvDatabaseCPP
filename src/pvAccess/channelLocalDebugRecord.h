/* channelLocalDebugRecord.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.04.18
 */
#ifndef CHANNELLOCALREBUGRECORD_H
#define CHANNELLOCALREBUGRECORD_H

#include <pv/channelProviderLocal.h>

namespace epics { namespace pvDatabase { 


class ChannelLocalDebugRecord;
typedef std::tr1::shared_ptr<ChannelLocalDebugRecord> ChannelLocalDebugRecordPtr;

class ChannelLocalDebugRecord :
    public PVRecord
{
public:
    POINTER_DEFINITIONS(ChannelLocalDebugRecord);
    static ChannelLocalDebugRecordPtr create(
        ChannelLocalDebugPtr const &channelLocalDebug,
        epics::pvData::String const & recordName);
    virtual ~ChannelLocalDebugRecord();
    virtual void destroy();
    virtual bool init();
    virtual void process();
private:
    ChannelLocalDebugRecord(
        ChannelLocalDebugPtr const &channelLocalDebug,
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    ChannelLocalDebugPtr channelLocalDebug;
    epics::pvData::PVIntPtr pvValue;
    bool isDestroyed;
};

}}

#endif  /* CHANNELLOCALREBUGRECORD_H */
