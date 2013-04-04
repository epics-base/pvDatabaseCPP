/* pvDatabase.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @date 2012.11.20
 */
#ifndef PVDATABASE_H
#define PVDATABASE_H

#include <list>
#include <deque>

#include <pv/pvAccess.h>
#include <pv/convert.h>

namespace epics { namespace pvDatabase { 

class PVRecord;
typedef std::tr1::shared_ptr<PVRecord> PVRecordPtr;

class PVRecordField;
typedef std::tr1::shared_ptr<PVRecordField> PVRecordFieldPtr;
typedef std::vector<PVRecordFieldPtr> PVRecordFieldPtrArray;
typedef std::tr1::shared_ptr<PVRecordFieldPtrArray> PVRecordFieldPtrArrayPtr;

class PVRecordStructure;
typedef std::tr1::shared_ptr<PVRecordStructure> PVRecordStructurePtr;

class PVRecordClient;
typedef std::tr1::shared_ptr<PVRecordClient> PVRecordClientPtr;

class PVListener;
typedef std::tr1::shared_ptr<PVListener> PVListenerPtr;

class PVDatabase;
typedef std::tr1::shared_ptr<PVDatabase> PVDatabasePtr;

class PVRecord :
     public epics::pvData::Requester,
     public std::tr1::enable_shared_from_this<PVRecord>
{
public:
    POINTER_DEFINITIONS(PVRecord);
    static PVRecordPtr create(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    PVRecord(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    virtual ~PVRecord();
    virtual void destroy();
    epics::pvData::String getRecordName();
    PVRecordStructurePtr getPVRecordStructure();
    PVRecordFieldPtr findPVRecordField(
        epics::pvData::PVFieldPtr const & pvField);
    bool addRequester(epics::pvData::RequesterPtr const & requester);
    bool removeRequester(epics::pvData::RequesterPtr const & requester);
    inline void lock_guard() { epics::pvData::Lock theLock(mutex); }
    void lock();
    void unlock();
    bool tryLock();
    void lockOtherRecord(PVRecordPtr const & otherRecord);
    bool addPVRecordClient(PVRecordClientPtr const & pvRecordClient);
    bool removePVRecordClient(PVRecordClientPtr const & pvRecordClient);
    void detachClients();
    bool addListener(PVListenerPtr const & pvListener);
    bool removeListener(PVListenerPtr const & pvListener);
    void beginGroupPut();
    void endGroupPut();
    epics::pvData::String getRequesterName() {return getRecordName();}
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    void message(
        PVRecordFieldPtr const & pvRecordField,
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    void toString(epics::pvData::StringBuilder buf);
    void toString(epics::pvData::StringBuilder buf,int indentLevel);
    //init MUST be called by derived class after derived class is constructed
    virtual bool init() {initPVRecord(); return true;}
    virtual void process() {}
protected:
    void initPVRecord();
    PVRecordPtr getPtrSelf()
    {
        return shared_from_this();
    }
private:
    PVRecordFieldPtr findPVRecordField(
        PVRecordStructurePtr const & pvrs,
        epics::pvData::PVFieldPtr const & pvField);
    epics::pvData::String recordName;
    epics::pvData::PVStructurePtr pvStructure;
    epics::pvData::ConvertPtr convert;
    PVRecordStructurePtr pvRecordStructure;
    std::list<PVListenerPtr> pvListenerList;
    std::list<PVRecordClientPtr> pvRecordClientList;
    std::list<epics::pvData::RequesterPtr> requesterList;
    epics::pvData::Mutex mutex;
    epics::pvData::Lock thelock;
    std::size_t depthGroupPut;
    bool isDestroyed;
};

class PVRecordField :
     public virtual epics::pvData::PostHandler,
     public std::tr1::enable_shared_from_this<PVRecordField>
{
public:
    POINTER_DEFINITIONS(PVRecordField);
    PVRecordField(
        epics::pvData::PVFieldPtr const & pvField,
        PVRecordStructurePtr const &parent,
        PVRecordPtr const & pvRecord);
    virtual ~PVRecordField();
    PVRecordStructurePtr getParent();
    epics::pvData::PVFieldPtr getPVField();
    epics::pvData::String getFullFieldName();
    epics::pvData::String getFullName();
    PVRecordPtr getPVRecord();
    bool addListener(PVListenerPtr const & pvListener);
    virtual void removeListener(PVListenerPtr const & pvListener);
    virtual void postPut();
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
protected:
    PVRecordFieldPtr getPtrSelf()
    {
        return shared_from_this();
    }
    virtual void init();
private:
    void callListener();

    std::list<PVListenerPtr> pvListenerList;
    epics::pvData::PVFieldPtr pvField;
    PVRecordStructurePtr parent;
    PVRecordPtr pvRecord;
    epics::pvData::String fullName;
    epics::pvData::String fullFieldName;
    friend class PVRecordStructure;
    friend class PVRecord;
};

class PVRecordStructure : public PVRecordField {
public:
    POINTER_DEFINITIONS(PVRecordStructure);
    PVRecordStructure(
        epics::pvData::PVStructurePtr const &pvStructure,
        PVRecordStructurePtr const &parent,
        PVRecordPtr const & pvRecord);
    virtual ~PVRecordStructure();
    PVRecordFieldPtrArrayPtr getPVRecordFields();
    epics::pvData::PVStructurePtr getPVStructure();
    virtual void removeListener(PVListenerPtr const & pvListener);
    virtual void postPut();
protected:
    virtual void init();
private:
    epics::pvData::PVStructurePtr pvStructure;
    PVRecordFieldPtrArrayPtr pvRecordFields;
    friend class PVRecord;
};

class PVRecordClient {
public:
    POINTER_DEFINITIONS(PVRecordClient);
    virtual ~PVRecordClient() {}
    virtual void detach(PVRecordPtr const & pvRecord) = 0;
};

class PVListener :
    virtual public PVRecordClient
{
public:
    POINTER_DEFINITIONS(PVListener);
    virtual ~PVListener() {}
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField) = 0;
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField) = 0;
    virtual void beginGroupPut(PVRecordPtr const & pvRecord) = 0;
    virtual void endGroupPut(PVRecordPtr const & pvRecord) = 0;
};

class PVDatabase : virtual public epics::pvData::Requester {
public:
    POINTER_DEFINITIONS(PVDatabase);
    static PVDatabasePtr getMaster();
    virtual ~PVDatabase();
    PVRecordPtr findRecord(epics::pvData::String const& recordName);
    bool addRecord(PVRecordPtr const & record);
    bool removeRecord(PVRecordPtr const & record);
    virtual epics::pvData::String getRequesterName();
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
private:
    PVDatabase();
};

}}

#endif  /* PVDATABASE_H */
