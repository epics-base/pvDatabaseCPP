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

#include <pv/pvAccess.h>

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


class PVRecord 
{
public:
    POINTER_DEFINITIONS(PVRecord);
    PVRecord(
        epics::pvData::String const & recordName,
        epics::pvData::PVStructurePtr const & pvStructure);
    virtual ~PVRecord();
    virtual void process(
        RecordProcessRequesterPtr const &recordProcessRequester) = 0;
    virtual bool isSynchronous() = 0;
    epics::pvData::String getRecordName();
    PVRecordStructurePtr getPVRecordStructure();
    PVRecordFieldPtr findPVRecordField(
        epics::pvData::PVFieldPtr const & pvField);
    void lock();
    void unlock();
    void registerClient(PVRecordClientPtr const & pvRecordClient);
    void unregisterClient(PVRecordClientPtr const & pvRecordClient);
    void detachClients();
    void beginGroupPut();
    void endGroupPut();
    void registerListener(PVListenerPtr const & pvListener);
    void unregisterListener(PVListenerPtr const & pvListener);
    void removeEveryListener();
    epics::pvData::Status processRequest();
    void queueProcessRequest(
        RecordProcessRequesterPtr const &recordProcessRequester);
    void addRequester(epics::pvData::RequesterPtr const & requester);
    void removeRequester(epics::pvData::RequesterPtr const & requester);
    void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
    epics::pvData::String toString();
    epics::pvData::String toString(int indentLevel);
private:
    epics::pvData::String recordName;
    epics::pvData::PVStructurePtr pvStructure;
};

class PVRecordField {
public:
    POINTER_DEFINITIONS(PVRecordField);
    PVRecordField(
        epics::pvData::PVFieldPtr const & pvField,
        PVRecordPtr const & pvRecord);
    virtual ~PVRecordField();
    PVRecordStructurePtr getParent();
    epics::pvData::PVFieldPtr getPVField();
    epics::pvData::String getFullFieldName();
    epics::pvData::String getFullName();
    PVRecordPtr getPVRecord();
    bool addListener(PVListenerPtr const & pvListener);
    void removeListener(PVListenerPtr const & pvListener);
    void postPut();
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
};

class PVRecordStructure : public PVRecordField {
public:
    POINTER_DEFINITIONS(PVRecordStructure);
    PVRecordStructure(
        epics::pvData::PVStructurePtr const &pvStructure,
        PVRecordFieldPtrArrayPtr const &pvRecordField);
    virtual ~PVRecordStructure();
    PVRecordFieldPtrArrayPtr getPVRecordFields();
    epics::pvData::PVStructurePtr getPVStructure();
    virtual void message(
        epics::pvData::String const & message,
        epics::pvData::MessageType messageType);
};

class PVListener {
public:
    POINTER_DEFINITIONS(PVListener);
    virtual ~PVListener();
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
    virtual ~RecordProcessRequester();
    virtual void becomeProcessor() = 0;
    virtual void recordProcessResult(epics::pvData::Status status) = 0;
    virtual void recordProcessComplete() = 0;
};

class PVRecordClient {
    POINTER_DEFINITIONS(PVRecordClient);
    virtual ~PVRecordClient();
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
private:
    PVDatabase();
};

}}

#endif  /* PVDATABASE_H */
