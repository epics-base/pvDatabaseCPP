/* pvDatabase.h */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author mrk
 * @data 2012.11.20
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

class PVListener;
typedef std::tr1::shared_ptr<PVListener> PVListenerPtr;

class RecordProcessRequester;
typedef std::tr1::shared_ptr<RecordProcessRequester> RecordProcessRequesterPtr;

class PVRecordClient;
typedef std::tr1::shared_ptr<PVRecordClient> PVRecordClientPtr;

class PVDatabase;
typedef std::tr1::shared_ptr<PVDatabase> PVDatabasePtr;


class PVRecord :
     public epics::pvData::Requester,
     public std::tr1::enable_shared_from_this<PVRecord>
{
public:
    POINTER_DEFINITIONS(PVRecord);
    PVRecord(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    virtual ~PVRecord();
    virtual void process(
        RecordProcessRequesterPtr const &recordProcessRequester,
        bool alreadyLocked) = 0;
    virtual bool isSynchronous() = 0;
    epics::pvData::String getRecordName();
    PVRecordStructurePtr getPVRecordStructure();
    PVRecordFieldPtr findPVRecordField(
        epics::pvData::PVFieldPtr const & pvField);
    void lock();
    void unlock();
    bool tryLock();
    bool addPVRecordClient(PVRecordClientPtr const & pvRecordClient);
    bool removePVRecordClient(PVRecordClientPtr const & pvRecordClient);
    void detachClients();
    void beginGroupPut();
    void endGroupPut();
    bool addListener(PVListenerPtr const & pvListener);
    bool removeListener(PVListenerPtr const & pvListener);
    void queueProcessRequest(
        RecordProcessRequesterPtr const &recordProcessRequester);
    void dequeueProcessRequest(
        RecordProcessRequesterPtr const &recordProcessRequester);
    bool addRequester(epics::pvData::RequesterPtr const & requester);
    bool removeRequester(epics::pvData::RequesterPtr const & requester);
    virtual epics::pvData::String getRequesterName() {return getRecordName();}
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    void message(
        PVRecordFieldPtr const & pvRecordField,
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    void toString(epics::pvData::StringBuilder buf);
    void toString(epics::pvData::StringBuilder buf,int indentLevel);
    //init MUST be called after derived class is constructed
    void init();
protected:
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
    std::deque<RecordProcessRequesterPtr> processRequesterQueue;
    std::list<PVListenerPtr> pvListenerList;
    std::list<PVRecordClientPtr> pvRecordClientList;
    std::list<epics::pvData::RequesterPtr> requesterList;
    epics::pvData::Mutex mutex;
    epics::pvData::Lock thelock;
    int depthGroupPut;
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

class PVListener {
public:
    POINTER_DEFINITIONS(PVListener);
    virtual ~PVListener() {}
    virtual void dataPut(PVRecordFieldPtr const & pvRecordField) = 0;
    virtual void dataPut(
        PVRecordStructurePtr const & requested,
        PVRecordFieldPtr const & pvRecordField) = 0;
    virtual void beginGroupPut(PVRecordPtr const & pvRecord) = 0;
    virtual void endGroupPut(PVRecordPtr const & pvRecord) = 0;
    virtual void unlisten(PVRecordPtr const & pvRecord) = 0;
};

class RecordProcessRequester :
    virtual public epics::pvData::Requester
{
public:
    POINTER_DEFINITIONS(RecordProcessRequester);
    virtual ~RecordProcessRequester() {}
    virtual void becomeProcessor() = 0;
    virtual void recordProcessResult(epics::pvData::Status status) = 0;
    virtual void recordProcessComplete() = 0;
};

class PVRecordClient {
public:
    POINTER_DEFINITIONS(PVRecordClient);
    virtual ~PVRecordClient() {}
    virtual void detach(PVRecordPtr const & pvRecord) = 0;
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
