/* pvCopy.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2013.03.25
 */
#ifndef PVCOPY_H
#define PVCOPY_H
#include <string>
#include <stdexcept>
#include <memory>

#include <pv/pvType.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>

#include <pv/pvDatabase.h>

namespace epics { namespace pvDatabase { 

class PVCopy;
typedef std::tr1::shared_ptr<PVCopy> PVCopyPtr;
class PVCopyMonitor;
typedef std::tr1::shared_ptr<PVCopyMonitor> PVCopyMonitorPtr;
class PVCopyMonitorRequester;
typedef std::tr1::shared_ptr<PVCopyMonitorRequester> PVCopyMonitorRequesterPtr;
class SharePVScalarArray;
typedef std::tr1::shared_ptr<SharePVScalarArray> SharePVScalarArrayPtr;
class SharePVStructureArray;
typedef std::tr1::shared_ptr<SharePVStructureArray> SharePVStructureArrayPtr;

struct CopyNode;
typedef std::tr1::shared_ptr<CopyNode> CopyNodePtr;
struct CopyRecordNode;
typedef std::tr1::shared_ptr<CopyRecordNode> CopyRecordNodePtr;
struct CopyStructureNode;
typedef std::tr1::shared_ptr<CopyStructureNode> CopyStructureNodePtr;


class PVCopy : 
    public std::tr1::enable_shared_from_this<PVCopy>
{
public:
    POINTER_DEFINITIONS(PVCopy);
    static PVCopyPtr create(
        PVRecordPtr const &pvRecord,
        epics::pvData::PVStructurePtr const &pvRequest,
        epics::pvData::String const & structureName);
    virtual ~PVCopy(){}
    PVRecordPtr getPVRecord();
    epics::pvData::StructureConstPtr getStructure();
    epics::pvData::PVStructurePtr createPVStructure();
    std::size_t getCopyOffset(PVRecordFieldPtr const  &recordPVField);
    std::size_t getCopyOffset(
        PVRecordStructurePtr const  &recordPVStructure,
        PVRecordFieldPtr const  &recordPVField);
    PVRecordFieldPtr getRecordPVField(std::size_t structureOffset);
    void initCopy(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet,
        bool lockRecord);
    void updateCopySetBitSet(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet,
        bool lockRecord);
    void updateCopyFromBitSet(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet,
        bool lockRecord);
    void updateRecord(
        epics::pvData::PVStructurePtr const  &copyPVStructure,
        epics::pvData::BitSetPtr const  &bitSet,
        bool lockRecord);
    PVCopyMonitorPtr createPVCopyMonitor(
        PVCopyMonitorRequesterPtr const  &pvCopyMonitorRequester);
    epics::pvData::PVStructurePtr getOptions(
        epics::pvData::PVStructurePtr const &copyPVStructure,std::size_t fieldOffset);
    epics::pvData::String dump();
private:
    PVCopyPtr getPtrSelf()
    {
        return shared_from_this();
    }

    PVRecordPtr pvRecord;
    epics::pvData::StructureConstPtr structure;
    CopyNodePtr headNode;
    epics::pvData::PVStructurePtr cacheInitStructure;
private:
    PVCopy(PVRecordPtr const &pvRecord);
    bool init(epics::pvData::PVStructurePtr const &pvRequest);
    epics::pvData::String dump(
        epics::pvData::String const &value,
        CopyNodePtr const &node,
        int indentLevel);
    epics::pvData::String getFullName(
        epics::pvData::PVStructurePtr const &pvFromRequest,
        epics::pvData::String  const &nameFromRecord);
    epics::pvData::PVStructurePtr getSubStructure(
        epics::pvData::PVStructurePtr const &pvFromRequest,
        epics::pvData::String  const &nameFromRecord);
    epics::pvData::PVStructurePtr getOptions(
        epics::pvData::PVStructurePtr const &pvFromRequest,
        epics::pvData::String  const &nameFromRecord);
    epics::pvData::StructureConstPtr createStructure(
        epics::pvData::PVStructurePtr const &pvRecord,
        epics::pvData::PVStructurePtr const &pvFromRequest);
    epics::pvData::FieldConstPtr createField(
        epics::pvData::PVStructurePtr const &pvRecord,
        epics::pvData::PVStructurePtr const &pvFromRequest);
    CopyNodePtr createStructureNodes(
        PVRecordStructurePtr const &pvRecordStructure,
        epics::pvData::PVStructurePtr const &pvFromRequest,
        epics::pvData::PVFieldPtr const &pvFromField);
    void referenceImmutable(
        epics::pvData::PVFieldPtr const &pvField,
        CopyNodePtr const & node);
    void referenceImmutable(
        epics::pvData::PVFieldPtr const &copyPVField,
        PVRecordFieldPtr const &recordPVField);
    void makeShared(
        epics::pvData::PVFieldPtr const &copyPVField,
        PVRecordFieldPtr const &recordPVField);
    void updateStructureNodeSetBitSet(
        epics::pvData::PVStructurePtr const &pvCopy,
        CopyStructureNodePtr const &structureNode,
        epics::pvData::BitSetPtr const &bitSet);
    void updateSubFieldSetBitSet(
        epics::pvData::PVFieldPtr const &pvCopy,
        PVRecordFieldPtr const &pvRecord,
        epics::pvData::BitSetPtr const &bitSet);
    void updateStructureNodeFromBitSet(
        epics::pvData::PVStructurePtr const &pvCopy,
        CopyStructureNodePtr const &structureNode,
        epics::pvData::BitSetPtr const &bitSet,
        bool toCopy,
        bool doAll);
    void updateSubFieldFromBitSet(
        epics::pvData::PVFieldPtr const &pvCopy,
        PVRecordFieldPtr const &pvRecordField,
        epics::pvData::BitSetPtr const &bitSet,
        bool toCopy,
        bool doAll);
    CopyRecordNodePtr getCopyOffset(
        CopyStructureNodePtr const &structureNode,
        PVRecordFieldPtr const &recordPVField);
    CopyRecordNodePtr getRecordNode(
        CopyStructureNodePtr const &structureNode,
        std::size_t structureOffset);
    
};

class PVCopyMonitor : 
    public std::tr1::enable_shared_from_this<PVCopyMonitor>
{
public:
    POINTER_DEFINITIONS(PVCopyMonitor);
    virtual ~PVCopyMonitor();
    void startMonitoring(
        epics::pvData::BitSetPtr const &changeBitSet,
        epics::pvData::BitSetPtr const &overrunBitSet);
    void stopMonitoring();
    void switchBitSets(
        epics::pvData::BitSetPtr const &newChangeBitSet,
        epics::pvData::BitSetPtr const &newOverrunBitSet, bool lockRecord);
private:
    PVCopyMonitorPtr getPtrSelf()
    {
        return shared_from_this();
    }
    PVCopyMonitor();
    friend class PVCopy;
    // TBD
};

class PVCopyMonitorRequester : 
    public std::tr1::enable_shared_from_this<PVCopyMonitorRequester>
{
public:
    POINTER_DEFINITIONS(PVCopyMonitorRequester);
    virtual void dataChanged() = 0;
    virtual void unlisten() = 0;
private:
    PVCopyMonitorRequesterPtr getPtrSelf()
    {
        return shared_from_this();
    }
};


#ifdef MUSTIMPLEMENT
template<typename T>
class SharePVScalarArray :
    public epics::pvData::PVScalarArray
{
public:
    POINTER_DEFINITIONS(SharePVScalarArray)
    typedef T  value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef epics::pvData::PVArrayData<T> ArrayDataType;

    SharePVScalarArray(
        PVRecordFieldPtr const &pvRecordField,
        epics::pvData::PVStructurePtr const &parent,
        epics::pvData::PVScalarArrayPtr const &pvShare);
    virtual ~SharePVScalarArray();
    void lockShare() const;
    void unlockShare() const;
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    virtual void setImmutable();
    bool isImmutable();
    virtual void setCapacity(std::size_t capacity);
    void setLength(std::size_t length);
    std::size_t getCapacity() const;
    std::size_t getLength() const;
    bool isCapacityMutable();
    void setCapacityMutable(bool isMutable);
    virtual std::size_t get(std::size_t offset, std::size_t length, ArrayDataType &data) = 0;
    virtual std::size_t put(std::size_t offset,std::size_t length, pointer from, std::size_t fromOffset) = 0;
    virtual void shareData(pointer value,std::size_t capacity,std::size_t length) = 0;
    virtual bool equals(epics::pvData::PVFieldPtr const &pv);
    virtual void serialize(
        epics::pvData::ByteBufferPtr *pbuffer,
        epics::pvData::SerializableControl *pflusher) const;
    virtual void deserialize(
        epics::pvData::ByteBuffer *pbuffer,
        epics::pvData::DeserializableControl *pflusher);
    virtual void serialize(
        epics::pvData::ByteBuffer *pbuffer,
        epics::pvData::SerializableControl *pflusher, std::size_t offset, std::size_t count) const;
private:
    PVRecordField &pvRecordField;
};

typedef SharePVScalarArray<bool> SharePVBooleanArray;
typedef SharePVScalarArray<epics::pvData::int8> SharePVByteArray;
typedef SharePVScalarArray<epics::pvData::int16> SharePVShortArray;
typedef SharePVScalarArray<epics::pvData::int32> SharePVIntArray;
typedef SharePVScalarArray<epics::pvData::int64> SharePVLongArray;
typedef SharePVScalarArray<float> SharePVFloatArray;
typedef SharePVScalarArray<double> SharePVDoubleArray;
typedef SharePVScalarArray<epics::pvData::String> SharePVStringArray;


class SharePVStructureArray :
    public epics::pvData::PVStructureArray
{
public:
    POINTER_DEFINITIONS(SharePVStructureArray)
    SharePVStructureArray(
        PVRecordFieldPtr const &pvRecordField,
        epics::pvData::PVStructurePtr const &parent,
        epics::pvData::PVStructureArrayPtr const &pvShare);
    virtual ~SharePVStructureArray();
    virtual epics::pvData::StructureArrayConstPtr getStructureArray();
    void lockShare() const;
    void unlockShare() const;
    virtual void message(
        epics::pvData::String const &message,
        epics::pvData::MessageType messageType);
    virtual void setImmutable();
    bool isImmutable();
    virtual void setCapacity(std::size_t capacity);
    void setLength(std::size_t length);
    std::size_t getCapacity() const;
    std::size_t getLength() const;
    bool isCapacityMutable();
    void setCapacityMutable(bool isMutable);
    virtual void shareData(
        epics::pvData::PVStructurePtrArrayPtr const & value,
        std::size_t capacity,std::size_t length);
    virtual std::size_t get(std::size_t offset, std::size_t length,
        epics::pvData::StructureArrayData &data);
    virtual std::size_t put(std::size_t offset,std::size_t length,
        epics::pvData::PVStructurePtrArrayPtr const & from,
        std::size_t fromOffset);
    virtual bool equals(epics::pvData::PVField & &pv);
    virtual void serialize(
        epics::pvData::ByteBuffer *pbuffer,
        epics::pvData::SerializableControl *pflusher) const;
    virtual void deserialize(
        epics::pvData::ByteBuffer *pbuffer,
        epics::pvData::DeserializableControl *pflusher);
    virtual void serialize(
        epics::pvData::ByteBuffer *pbuffer,
        epics::pvData::SerializableControl *pflusher,
        std::size_t offset, std::size_t count) const;
private:
    PVRecordField &pvRecordField;
};
#endif


}}

#endif  /* PVCOPY_H */
