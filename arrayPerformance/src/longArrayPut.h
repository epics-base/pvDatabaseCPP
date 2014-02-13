/* longArrayPut.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.08.09
 */
#ifndef LONGARRAYPUT_H
#define LONGARRAYPUT_H

#include <shareLib.h>

#include <pv/event.h>
#include <pv/lock.h>
#include <pv/standardPVField.h>
#include <pv/timeStamp.h>
#include <pv/pvTimeStamp.h>
#include <pv/pvAccess.h>

namespace epics { namespace pvDatabase { 


class LongArrayPut;
typedef std::tr1::shared_ptr<LongArrayPut> LongArrayPutPtr;


class LongArrayChannelPut;
typedef std::tr1::shared_ptr<LongArrayChannelPut> LongArrayChannelPutPtr;

class epicsShareClass  LongArrayPut :
    public std::tr1::enable_shared_from_this<LongArrayPut>
{
public:
    POINTER_DEFINITIONS(LongArrayPut);
    static LongArrayPutPtr create(
        epics::pvData::String const & providerName,
        epics::pvData::String const & channelName,
        size_t arraySize = 100,
        int iterBetweenCreateChannel = 0,
        int iterBetweenCreateChannelPut = 0,
        double delayTime = 0.0);
    ~LongArrayPut();
    void destroy();
private:
    LongArrayPutPtr getPtrSelf()
    {
        return shared_from_this();
    }
    LongArrayPut(
        epics::pvData::String const & providerName,
        epics::pvData::String const & channelName,
        size_t arraySize,
        int iterBetweenCreateChannel,
        int iterBetweenCreateChannelPut,
        double delayTime);
    bool init();

    epics::pvData::String providerName;
    epics::pvData::String channelName;
    size_t arraySize;
    int iterBetweenCreateChannel;
    int iterBetweenCreateChannelPut;
    double delayTime;
    LongArrayChannelPutPtr longArrayChannelPut;
};


}}

#endif  /* LONGARRAYPUT_H */
