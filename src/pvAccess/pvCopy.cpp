/* pvCopy.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvData is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/**
 * @author Marty Kraimer
 * @date 2013.04
 */
#include <string>
#include <stdexcept>
#include <memory>
#include <sstream>

#include <pv/pvCopy.h>

namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::size_t;

static PVCopyPtr NULLPVCopy;
static FieldConstPtr NULLField;
static StructureConstPtr NULLStructure;
static PVStructurePtr NULLPVStructure;
static CopyNodePtr NULLCopyNode;
static CopyRecordNodePtr NULLCopyRecordNode;

struct CopyNode {
    CopyNode()
    : isStructure(false),
      structureOffset(0),
      nfields(0),
      shareData(false)
    {}
    bool isStructure;
    size_t structureOffset; // In the copy
    size_t nfields;
    bool shareData;
    PVStructurePtr options;
};
    
struct CopyRecordNode : public CopyNode{
    PVRecordFieldPtr recordPVField;
};

typedef std::vector<CopyNodePtr> CopyNodePtrArray;
typedef std::tr1::shared_ptr<CopyNodePtrArray> CopyNodePtrArrayPtr;
    
struct CopyStructureNode : public  CopyNode {
    CopyNodePtrArrayPtr nodes;
};

PVCopyPtr PVCopy::create(
    PVRecordPtr const &pvRecord, 
    PVStructurePtr const &pvRequest, 
    String const & structureName)
{
    PVStructurePtr pvStructure(pvRequest);
    if(structureName.size()>0) {
        if(pvRequest->getStructure()->getNumberFields()>0) {
            pvStructure = pvRequest->getStructureField(structureName);
            if(pvStructure.get()==NULL) return NULLPVCopy;
        }
    } else if(pvStructure->getSubField("field")!=NULL) {
        pvStructure = pvRequest->getStructureField("field");
    }
    PVCopyPtr pvCopy = PVCopyPtr(new PVCopy(pvRecord));
    bool result = pvCopy->init(pvStructure);
    if(!result) pvCopy.reset();
    return pvCopy;
}

PVCopy::PVCopy(PVRecordPtr const &pvRecord)
: pvRecord(pvRecord)
{
}

PVRecordPtr PVCopy::getPVRecord()
{
    return pvRecord;
}

StructureConstPtr PVCopy::getStructure()
{
    return structure;
}

PVStructurePtr PVCopy::createPVStructure()
{
    if(cacheInitStructure.get()!=NULL) {
        PVStructurePtr save = cacheInitStructure;
        cacheInitStructure.reset();
        return save;
    }
    PVStructurePtr pvStructure = 
        getPVDataCreate()->createPVStructure(structure);
    return pvStructure;
}

size_t PVCopy::getCopyOffset(PVRecordFieldPtr const &recordPVField)
{
    if(!headNode->isStructure) {
        CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(headNode);
        if((recordNode->recordPVField.get())==recordPVField.get()) {
             return headNode->structureOffset;
        }
        return String::npos;
    }
    CopyStructureNodePtr node = static_pointer_cast<CopyStructureNode>(headNode);
    CopyRecordNodePtr recordNode = getCopyOffset(node,recordPVField);
    if(recordNode.get()!=NULL) return recordNode->structureOffset;
    return String::npos;
}

size_t PVCopy::getCopyOffset(
    PVRecordStructurePtr const  &recordPVStructure,
    PVRecordFieldPtr const  &recordPVField)
{
    CopyRecordNodePtr recordNode;
    if(!headNode->isStructure) {
        recordNode = static_pointer_cast<CopyRecordNode>(headNode);
        if(recordNode->recordPVField.get()!=recordPVStructure.get()) return String::npos;
    } else {
        CopyStructureNodePtr node = static_pointer_cast<CopyStructureNode>(headNode);
        recordNode = getCopyOffset(node,recordPVField);
    }
    if(recordNode.get()==NULL) return String::npos;
    size_t diff = recordPVField->getPVField()->getFieldOffset()
        - recordPVStructure->getPVStructure()->getFieldOffset();
    return recordNode->structureOffset + diff;
}

PVRecordFieldPtr PVCopy::getRecordPVField(size_t structureOffset)
{
    CopyRecordNodePtr recordNode;
    if(!headNode->isStructure) {
        recordNode = static_pointer_cast<CopyRecordNode>(headNode);
    } else {
        CopyStructureNodePtr node = static_pointer_cast<CopyStructureNode>(headNode);
        recordNode = getRecordNode(node,structureOffset);
    }
    if(recordNode.get()==NULL) {
        throw std::invalid_argument(
            "PVCopy::getRecordPVField: setstructureOffset not valid");
    }
    size_t diff = structureOffset - recordNode->structureOffset;
    PVRecordFieldPtr pvRecordField = recordNode->recordPVField;
    if(diff==0) return pvRecordField;
    PVStructurePtr pvStructure
        = static_pointer_cast<PVStructure>(pvRecordField->getPVField());
    PVFieldPtr pvField = pvStructure->getSubField(
        pvRecordField->getPVField()->getFieldOffset() + diff);
    return pvRecord->findPVRecordField(pvField);
}

void PVCopy::initCopy(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet,
    bool lockRecord)
{
    bitSet->clear();
    bitSet->set(0);
    updateCopyFromBitSet(copyPVStructure,bitSet,lockRecord);
}

void PVCopy::updateCopySetBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet,
    bool lockRecord)
{
    if(lockRecord) pvRecord->lock();
    try {
        if(headNode->isStructure) {
            CopyStructureNodePtr node = static_pointer_cast<CopyStructureNode>(headNode);
            updateStructureNodeSetBitSet(copyPVStructure,node,bitSet);
        } else {
            CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(headNode);
            PVRecordFieldPtr pvRecordField= recordNode->recordPVField;
            PVFieldPtr copyPVField = copyPVStructure;
            PVFieldPtrArray const & pvCopyFields = copyPVStructure->getPVFields();
            if(pvCopyFields.size()==1) {
                copyPVField = pvCopyFields[0];
            }
            PVFieldPtr pvField = pvRecordField->getPVField();
            if(pvField->getField()->getType()==epics::pvData::structure) {
                updateSubFieldSetBitSet(copyPVField,pvRecordField,bitSet);
                return;
            }
            if(pvCopyFields.size()!=1) {
                throw std::logic_error("Logic error");
            }
            ConvertPtr convert = getConvert();
            bool isEqual = convert->equals(copyPVField,pvField);
            if(!isEqual) {
                convert->copy(pvField, copyPVField);
                bitSet->set(copyPVField->getFieldOffset());
            }
        }
        if(lockRecord) pvRecord->unlock();
    } catch(...) {
        if(lockRecord) pvRecord->unlock();
        throw;
    }
}

void PVCopy::updateCopyFromBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet,
    bool lockRecord)
{
    bool doAll = bitSet->get(0);
    if(lockRecord) pvRecord->lock();
    try {
        if(headNode->isStructure) {
            CopyStructureNodePtr node = static_pointer_cast<CopyStructureNode>(headNode);
            updateStructureNodeFromBitSet(copyPVStructure,node,bitSet,true,doAll);
        } else {
            CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(headNode);
            PVFieldPtrArray const & pvCopyFields = copyPVStructure->getPVFields();
            if(pvCopyFields.size()==1) {
                updateSubFieldFromBitSet(
                    pvCopyFields[0],
                    recordNode->recordPVField,bitSet,
                    true,doAll);
            } else {
                updateSubFieldFromBitSet(
                    copyPVStructure,
                    recordNode->recordPVField,bitSet,
                    true,doAll);
            }
        }
        if(lockRecord) pvRecord->unlock();
    } catch(...) {
        if(lockRecord) pvRecord->unlock();
        throw;
    }
}

void PVCopy::updateRecord(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet,
    bool lockRecord)
{
    bool doAll = bitSet->get(0);
    if(lockRecord) pvRecord->lock();
    try {
        pvRecord->beginGroupPut();
        if(headNode->isStructure) {
            CopyStructureNodePtr node =
                static_pointer_cast<CopyStructureNode>(headNode);
            updateStructureNodeFromBitSet(
                copyPVStructure,node,bitSet,false,doAll);
        } else {
            CopyRecordNodePtr recordNode =
                static_pointer_cast<CopyRecordNode>(headNode);
            PVFieldPtrArray const & pvCopyFields =
                copyPVStructure->getPVFields();
            if(pvCopyFields.size()==1) {
                updateSubFieldFromBitSet(
                    pvCopyFields[0],
                    recordNode->recordPVField,bitSet,
                    false,doAll);
            } else {
                updateSubFieldFromBitSet(
                    copyPVStructure,
                    recordNode->recordPVField,bitSet,
                    false,doAll);
            }
        }
        pvRecord->endGroupPut();
        if(lockRecord) pvRecord->unlock();
    } catch(...) {
        if(lockRecord) pvRecord->unlock();
        throw;
    }
}

PVCopyMonitorPtr PVCopy::createPVCopyMonitor(
    PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester)
{
    PVCopyMonitorPtr pvCopyMonitor( new PVCopyMonitor(
        pvRecord,headNode,getPtrSelf(),pvCopyMonitorRequester));
    return pvCopyMonitor;
}

epics::pvData::String PVCopy::dump()
{
    String builder;
    dump(&builder,headNode,0);
    return builder;
}

void PVCopy::dump(String *builder,CopyNodePtr const &node,int indentLevel)
{
    getConvert()->newLine(builder,indentLevel);
    std::stringstream ss;
    ss << (node->isStructure ? "structureNode" : "recordNode");
    ss << " structureOffset " << node->structureOffset;
    ss << " nfields " << node->nfields;
    *builder +=  ss.str();
    PVStructurePtr options = node->options;
    if(options.get()!=NULL) {
        getConvert()->newLine(builder,indentLevel +1);
        options->toString(builder);
        getConvert()->newLine(builder,indentLevel);
    }
    if(!node->isStructure) {
        CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(node);
        String name = recordNode->recordPVField->getFullName();
        *builder += " recordField " + name;
        return;
    }
    CopyStructureNodePtr structureNode =
        static_pointer_cast<CopyStructureNode>(node);
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i<nodes->size(); ++i) {
        if((*nodes)[i].get()==NULL) {
            getConvert()->newLine(builder,indentLevel +1);
            ss.str("");
            ss << "node[" << i << "] is null";
            *builder += ss.str();
            continue;
        }
        dump(builder,(*nodes)[i],indentLevel+1);
    }
}

bool PVCopy::init(epics::pvData::PVStructurePtr const &pvRequest)
{
    PVRecordStructurePtr pvRecordStructure = pvRecord->getPVRecordStructure();
    size_t len = pvRequest->getPVFields().size();
    bool entireRecord = false;
    if(len==String::npos) entireRecord = true;
    PVStructurePtr pvOptions;
    if(len==1 && pvRequest->getSubField("_options")!=NULL) {
        pvOptions = pvRequest->getStructureField("_options");
    }
    if(entireRecord) {
        structure = pvRecordStructure->getPVStructure()->getStructure();
        CopyRecordNodePtr recordNode(new CopyRecordNode());
        headNode = recordNode;
        recordNode->options = pvOptions;
        recordNode->isStructure = false;
        recordNode->structureOffset = 0;
        recordNode->recordPVField = pvRecordStructure;
        return true;
    }
    structure = createStructure(pvRecordStructure->getPVStructure(),pvRequest);
    if(structure==NULL) return false;
    cacheInitStructure = createPVStructure();
    headNode = createStructureNodes(
        pvRecord->getPVRecordStructure(),
        pvRequest,
        cacheInitStructure);
    referenceImmutable(cacheInitStructure,headNode);
    return true;
}

epics::pvData::String PVCopy::dump(
    String const &value,
    CopyNodePtr const &node,
    int indentLevel)
{
    throw std::logic_error(String("Not Implemented"));
}

String PVCopy::getFullName(
    PVStructurePtr const &pvFromRequest,
    String  const &nameFromRecord)
{
    PVFieldPtrArray const & pvFields = pvFromRequest->getPVFields();
    String fullName = nameFromRecord;
    size_t len = pvFields.size();
    if(len==1) {
        String name = pvFields[0]->getFieldName();
        if(name.compare("_options")==0) return nameFromRecord;
        PVStructurePtr pvRequest = static_pointer_cast<PVStructure>(pvFields[0]);
        if(fullName.size()!=0) fullName += ".";
        fullName += pvRequest->getFieldName();
        return getFullName(pvRequest,fullName);
    }
    if(len==2) {
        PVFieldPtr subField;
        if((pvFields[0]->getFieldName().compare("_options"))==0) {
            subField = pvFields[1];
        } else if((pvFields[1]->getFieldName().compare("_options"))==0) {
            subField = pvFields[1];
        }
        if(subField.get()!=NULL) {
            PVStructurePtr pvRequest = static_pointer_cast<PVStructure>(subField);
            if(fullName.size()!=0) fullName += ".";
            fullName += subField->getFieldName();
            return getFullName(pvRequest,fullName);
        }
    }
    return nameFromRecord;
}

PVStructurePtr PVCopy::getSubStructure(
    PVStructurePtr const &xxx,
    String  const &yyy)
{
    PVStructurePtr pvFromRequest = xxx;
    String nameFromRecord = yyy;
    PVFieldPtrArray const & pvFields = pvFromRequest->getPVFields();
    size_t len = pvFields.size();
    if(len==1) {
        pvFromRequest = static_pointer_cast<PVStructure>(pvFields[0]);
        if(pvFromRequest->getFieldName().compare("_options")==0) {
            return pvFromRequest;
        }
        if(nameFromRecord.size()!=0) nameFromRecord += ".";
        nameFromRecord += pvFromRequest->getFieldName();
        return getSubStructure(pvFromRequest,nameFromRecord);
    }
    if(len==2) {
        PVFieldPtr subField;
        if(pvFields[0]->getFieldName().compare("_options")==0) {
            subField = pvFields[1];
        } else if(pvFields[1]->getFieldName().compare("_options")==0) {
            subField = pvFields[1];
        }
        if(subField.get()!=NULL) {
            if(nameFromRecord.size()!=0) nameFromRecord += ".";
            nameFromRecord += subField->getFieldName();
            pvFromRequest = static_pointer_cast<PVStructure>(subField);
            return getSubStructure(pvFromRequest,nameFromRecord);
        }
    }
    return pvFromRequest;
}

PVStructurePtr PVCopy::getOptions(
    PVStructurePtr const &xxx,
    String  const &yyy)
{
    PVStructurePtr pvFromRequest = xxx;
    String nameFromRecord = yyy;
    PVFieldPtrArray const & pvFields = pvFromRequest->getPVFields();
    size_t len = pvFields.size();
    if(len==1) {
        pvFromRequest = static_pointer_cast<PVStructure>(pvFields[0]);
        if(pvFromRequest->getFieldName().compare("_options")==0) {
            return pvFromRequest;
        }
        if(nameFromRecord.size()!=0) nameFromRecord += ".";
        nameFromRecord += pvFromRequest->getFieldName();
        return getSubStructure(pvFromRequest,nameFromRecord);
    }

    if(len==2) {
        if(pvFields[0]->getFieldName().compare("_options")==0) {
            pvFromRequest = static_pointer_cast<PVStructure>(pvFields[0]);
            return pvFromRequest;
        } else if(pvFields[1]->getFieldName().compare("_options")==0) {
            pvFromRequest = static_pointer_cast<PVStructure>(pvFields[1]);
            return pvFromRequest;
        }
        
    }
    return NULLPVStructure;
}

StructureConstPtr PVCopy::createStructure(
    PVStructurePtr const &pvRecord,
    PVStructurePtr const  &pvFromRequest)
{
    if(pvFromRequest->getStructure()->getNumberFields()==0) {
        return pvRecord->getStructure();
    }
    FieldConstPtr field = createField(pvRecord,pvFromRequest);
    if(field.get()==NULL) return NULLStructure;
    if(field->getType()==epics::pvData::structure) {
        StructureConstPtr structure =
            static_pointer_cast<const Structure>(field);
        return structure;
    }
    StringArray fieldNames(1);
    FieldConstPtrArray fields(1);
    String name = getFullName(pvFromRequest,"");
    size_t ind = name.find_last_of('.');
    if(ind!=String::npos) name = String(name,ind+1);
    fieldNames[0] = name;
    fields[0] = field;
    return getFieldCreate()->createStructure(fieldNames, fields);
}

FieldConstPtr PVCopy::createField(
    PVStructurePtr const &xxx,
    PVStructurePtr const &yyy)
{
    PVStructurePtr pvRecord = xxx;
    PVStructurePtr pvFromRequest = yyy;
    PVFieldPtrArray const & pvFromRequestFields
         = pvFromRequest->getPVFields();
    StringArray const & fromRequestFieldNames
        = pvFromRequest->getStructure()->getFieldNames();
    size_t length = pvFromRequestFields.size();
    size_t number = 0;
    size_t indopt = -1;
    for(size_t i=0; i<length; i++) {
        if(fromRequestFieldNames[i].compare("_options")!=0) {
            number++;
        } else {
            indopt = i;
        }
    }
    if(number==0) return pvRecord->getStructure();
    if(number==1) {
        String nameFromRecord = "";
        nameFromRecord = getFullName(pvFromRequest,nameFromRecord);
        PVFieldPtr pvRecordField = pvRecord->getSubField(nameFromRecord);
        if(pvRecordField.get()==NULL) return NULLField;
        Type recordFieldType = pvRecordField->getField()->getType();
        if(recordFieldType!=epics::pvData::structure) {
            return pvRecordField->getField();
        }
        PVStructurePtr pvSubFrom = static_pointer_cast<PVStructure>(
             pvFromRequest->getSubField(nameFromRecord));
        PVFieldPtrArray const & pvs = pvSubFrom->getPVFields();
        length = pvs.size();
        number = 0;
        for(size_t i=0; i<length; i++) {
            if(!pvs[i]->getFieldName().compare("_options")==0) {
                number++;
            }
        }
        FieldConstPtrArray fields(1);
        StringArray fieldNames(1);
        fieldNames[0] = pvRecordField->getFieldName();
        if(number==0) {
            fields[0] = pvRecordField->getField();
        } else {
            PVStructurePtr zzz =
                static_pointer_cast<PVStructure>(pvRecordField);
            fields[0] = createField(zzz,pvSubFrom);
        }
        return getFieldCreate()->createStructure(fieldNames, fields);
    }
    FieldConstPtrArray fields; fields.reserve(number);
    StringArray fieldNames; fields.reserve(number);
    for(size_t i=0; i<length; i++) {
        if(i==indopt) continue;
        PVStructurePtr arg = static_pointer_cast<PVStructure>(
            pvFromRequestFields[i]);
        PVStructurePtr yyy = static_pointer_cast<PVStructure>(
            pvFromRequestFields[i]);
        String zzz = getFullName(yyy,"");
        String full = fromRequestFieldNames[i];
        if(zzz.size()>0) {
            full += "." + zzz;
            arg = getSubStructure(yyy,zzz);
        }
        PVFieldPtr pvRecordField = pvRecord->getSubField(full);
        if(pvRecordField.get()==NULL) continue;
        FieldConstPtr field = pvRecordField->getField();
        if(field->getType()!=epics::pvData::structure) {
            fieldNames.push_back(full);
            fields.push_back(field);
            continue;
        }
        FieldConstPtr xxx = createField(
            static_pointer_cast<PVStructure>(pvRecordField),arg);
        if(xxx.get()!=NULL) {
            fieldNames.push_back(fromRequestFieldNames[i]);
            fields.push_back(xxx);
        }
    }
    boolean makeValue = true;
    size_t indValue = String::npos;
    for(size_t i=0;i<fieldNames.size(); i++) {
        if(fieldNames[i].compare("value")==0) {
            if(indValue==String::npos) {
                indValue = i;
            } else {
                makeValue = false;
            }
        }
    }
    for(size_t i=0;i<fieldNames.size(); i++) {
        if(makeValue==true&&indValue==i) {
            fieldNames[i] = "value";
        } else {
            String xxx = fieldNames[i];
            size_t ind = xxx.find_first_of('.');
            if(ind!=String::npos) fieldNames[i] = String(xxx,0, ind);
        }
    }
    return getFieldCreate()->createStructure(fieldNames, fields);
}

CopyNodePtr PVCopy::createStructureNodes(
    PVRecordStructurePtr const &xxx,
    PVStructurePtr const &yyy,
    PVFieldPtr const &zzz)
{
    PVRecordStructurePtr pvRecordStructure = xxx;
    PVStructurePtr pvFromRequest = yyy;
    PVFieldPtr pvFromField = zzz;

    PVFieldPtrArray const &  pvFromRequestFields = pvFromRequest->getPVFields();
    StringArray const & fromRequestFieldNames =
        pvFromRequest->getStructure()->getFieldNames();
    size_t length = pvFromRequestFields.size();
    size_t number = 0;
    size_t indopt = -1;
    PVStructurePtr pvOptions;
    for(size_t i=0; i<length; i++) {
        if(fromRequestFieldNames[i].compare("_options")!=0) {
            number++;
        } else {
            indopt = i;
            pvOptions = static_pointer_cast<PVStructure>(pvFromRequestFields[i]);
        }
    }
    if(number==0) {
        CopyRecordNodePtr recordNode(new CopyRecordNode());
        recordNode->options = pvOptions;
        recordNode->isStructure = false;
        recordNode->recordPVField = pvRecordStructure;
        recordNode->nfields = pvFromField->getNumberFields();
        recordNode->structureOffset = pvFromField->getFieldOffset();
        return recordNode;
    }
    if(number==1) {
        String nameFromRecord = "";
        nameFromRecord = getFullName(pvFromRequest,nameFromRecord);
        PVFieldPtr pvField = pvRecordStructure->
            getPVStructure()->getSubField(nameFromRecord);
        if(pvField.get()==NULL) return NULLCopyNode;
        PVRecordFieldPtr pvRecordField = pvRecordStructure->
            getPVRecord()->findPVRecordField(pvField);
        size_t structureOffset = pvFromField->getFieldOffset();
        PVStructure *pvParent = pvFromField->getParent();
        if(pvParent==NULL) {
            structureOffset++;
        }
        CopyRecordNodePtr recordNode(new CopyRecordNode());
        recordNode->options = getOptions(pvFromRequest,nameFromRecord);
        recordNode->isStructure = false;
        recordNode->recordPVField = pvRecordField;
        recordNode->nfields = pvFromField->getNumberFields();
        recordNode->structureOffset = structureOffset;
        return recordNode;
    }
    CopyNodePtrArrayPtr nodes(new CopyNodePtrArray());
    nodes->reserve(number);
    PVStructurePtr pvFromStructure =
        static_pointer_cast<PVStructure>(pvFromField);
    PVFieldPtrArray const &  pvFromStructureFields =
         pvFromStructure->getPVFields();
    length = pvFromStructureFields.size();
    size_t indFromStructure = 0;
    for(size_t i= 0; i <pvFromRequestFields.size();i++) {
        if(i==indopt) continue;
        PVStructurePtr arg = static_pointer_cast<PVStructure>
            (pvFromRequestFields[i]);
        PVStructurePtr yyy = static_pointer_cast<PVStructure>
            (pvFromRequestFields[i]);
        String zzz = getFullName(yyy,"");
        String full = fromRequestFieldNames[i];
        if(zzz.size()>0) {
            full += "." + zzz;
            arg = getSubStructure(yyy,zzz);
        }
        PVFieldPtr pvField = pvRecordStructure->
            getPVStructure()->getSubField(full);
        if(pvField.get()==NULL) continue;
        PVRecordFieldPtr pvRecordField =
             pvRecordStructure->getPVRecord()->findPVRecordField(pvField);
        CopyNodePtr node;
        if(pvRecordField->getPVField()->getField()==
        pvFromStructureFields[indFromStructure]->getField()) {
            pvField = pvFromStructureFields[indFromStructure];
            CopyRecordNodePtr recordNode(new CopyRecordNode());
            recordNode->options = getOptions(yyy,zzz);
            recordNode->isStructure = false;
            recordNode->recordPVField = pvRecordField;
            recordNode->nfields = pvField->getNumberFields();
            recordNode->structureOffset = pvField->getFieldOffset();
            node = recordNode;
        } else {
            node = createStructureNodes(static_pointer_cast<PVRecordStructure>
                (pvRecordField),arg,pvFromStructureFields[indFromStructure]);
        }
        if(node.get()==NULL) continue;
        nodes->push_back(node);
        ++indFromStructure;
    }
    size_t len = nodes->size();
    if(len==String::npos) return NULLCopyNode;
    CopyStructureNodePtr structureNode(new CopyStructureNode());
    structureNode->isStructure = true;
    structureNode->nodes = nodes;
    structureNode->structureOffset = pvFromStructure->getFieldOffset();
    structureNode->nfields = pvFromStructure->getNumberFields();
    structureNode->options = pvOptions;
    return structureNode;
}

void PVCopy::referenceImmutable(
    PVFieldPtr const &pvField,
    CopyNodePtr const & node)

{
   if(node->isStructure) {
        CopyStructureNodePtr structureNode =
             static_pointer_cast<CopyStructureNode>(node);
        CopyNodePtrArrayPtr nodes = structureNode->nodes;
        PVStructurePtr pvStructure = static_pointer_cast<PVStructure>(pvField);
        for(size_t i=0; i<nodes->size(); i++) {
            CopyNodePtr nextNode = (*nodes)[i];
            referenceImmutable(
                pvStructure->getSubField(nextNode->structureOffset),
                nextNode);
        }
    } else {
        CopyRecordNodePtr recordNode = 
             static_pointer_cast<CopyRecordNode>(node);
        PVRecordFieldPtr recordPVField = recordNode->recordPVField;
        bool shareData = false;
        if(node->options.get()!=NULL) {
            PVFieldPtr pv = node->options->getSubField("_options");
            if(pv.get()!=NULL) {
                PVStructurePtr xxx = static_pointer_cast<PVStructure>(pv);
                pv = xxx->getSubField("shareData");
                if(pv.get()!=NULL) {
                    PVStringPtr yyy = xxx->getStringField("shareData");
                    shareData = (yyy->get().compare("true")==0) ? true : false;
                }
            }
        }
    	if(shareData) {
            makeShared(pvField,recordNode->recordPVField);
        } else {
            referenceImmutable(pvField,recordPVField);
        }
    }
}

void PVCopy::referenceImmutable(
    PVFieldPtr const &copyPVField,
    PVRecordFieldPtr const &recordPVField)
{
    if(recordPVField->getPVField()->getField()->getType()
    ==epics::pvData::structure) {
        if(copyPVField->getField()->getType()!=epics::pvData::structure) {
            throw std::logic_error(String("Logic error"));
        }
        PVStructurePtr pvStructure =
            static_pointer_cast<PVStructure>(copyPVField);
        PVFieldPtrArray const & copyPVFields = pvStructure->getPVFields();
        PVRecordStructurePtr pvRecordStructure =
            static_pointer_cast<PVRecordStructure>(recordPVField);
        PVRecordFieldPtrArrayPtr recordPVFields =
            pvRecordStructure->getPVRecordFields();
        for(size_t i=0; i<copyPVFields.size(); i++) {
            referenceImmutable(copyPVFields[i],(*recordPVFields)[i]);
        }
        return;
    }
    if(recordPVField->getPVField()->isImmutable()) {
        getConvert()->copy(recordPVField->getPVField(), copyPVField);
    }
}

void PVCopy::makeShared(
    PVFieldPtr const &copyPVField,
    PVRecordFieldPtr const &recordPVField)
{
    throw std::logic_error(String("Not Implemented"));
}

void PVCopy::updateStructureNodeSetBitSet(
    PVStructurePtr const &pvCopy,
    CopyStructureNodePtr const &structureNode,
    epics::pvData::BitSetPtr const &bitSet)
{
    for(size_t i=0; i<structureNode->nodes->size(); i++) {
        CopyNodePtr node = (*structureNode->nodes)[i];
        PVFieldPtr pvField = pvCopy->getSubField(node->structureOffset);
        if(node->isStructure) {
            PVStructurePtr xxx = static_pointer_cast<PVStructure>(pvField);
            CopyStructureNodePtr yyy =
                static_pointer_cast<CopyStructureNode>(node);
            updateStructureNodeSetBitSet(xxx,yyy,bitSet); 
        } else {
            CopyRecordNodePtr recordNode =
                static_pointer_cast<CopyRecordNode>(node);
            bool shareData = false;
            if(node->options.get()!=NULL) {
                PVFieldPtr pv = node->options->getSubField("_options");
                if(pv.get()!=NULL) {
                    PVStructurePtr xxx =
                        static_pointer_cast<PVStructure>(pv);
                    pv = xxx->getSubField("shareData");
                    if(pv.get()!=NULL) {
                        PVStringPtr yyy = xxx->getStringField("shareData");
                        shareData = (yyy->get().compare("true")==0) ? true : false;
                    }
                }
            }
            if(shareData) {
            	bitSet->set(pvField->getFieldOffset());
            } else {
                updateSubFieldSetBitSet(pvField,recordNode->recordPVField,bitSet);
            }
        }
    }
}

void PVCopy::updateSubFieldSetBitSet(
    PVFieldPtr const &pvCopy,
    PVRecordFieldPtr const &pvRecord,
    BitSetPtr const &bitSet)
{
    FieldConstPtr field = pvCopy->getField();
    Type type = field->getType();
    if(type!=epics::pvData::structure) {
        ConvertPtr convert = getConvert();
        bool isEqual = convert->equals(pvCopy,pvRecord->getPVField());
    	if(isEqual) {
    	    if(type==structureArray) {
    	        // always act as though a change occurred.
    	        // Note that array elements are shared.
		bitSet->set(pvCopy->getFieldOffset());
    	    }
    	}
        if(isEqual) return;
        convert->copy(pvRecord->getPVField(), pvCopy);
        bitSet->set(pvCopy->getFieldOffset());
        return;
    }
    PVStructurePtr pvCopyStructure = static_pointer_cast<PVStructure>(pvCopy);
    PVFieldPtrArray const & pvCopyFields = pvCopyStructure->getPVFields();
    PVRecordStructurePtr pvRecordStructure =
        static_pointer_cast<PVRecordStructure>(pvRecord);
    PVRecordFieldPtrArrayPtr pvRecordFields =
        pvRecordStructure->getPVRecordFields();
    size_t length = pvCopyFields.size();
    for(size_t i=0; i<length; i++) {
        updateSubFieldSetBitSet(pvCopyFields[i],(*pvRecordFields)[i],bitSet);
    }
}

void PVCopy::updateStructureNodeFromBitSet(
    PVStructurePtr const &pvCopy,
    CopyStructureNodePtr const &structureNode,
    BitSetPtr const &bitSet,
    bool toCopy,
    bool doAll)
{
    size_t offset = structureNode->structureOffset;
    if(!doAll) {
        size_t nextSet = bitSet->nextSetBit(offset);
        if(nextSet==String::npos) return;
    }
    if(offset>=pvCopy->getNextFieldOffset()) return;
    if(!doAll) doAll = bitSet->get(offset);
    CopyNodePtrArrayPtr  nodes = structureNode->nodes;
    for(size_t i=0; i<nodes->size(); i++) {
        CopyNodePtr node = (*nodes)[i];
        PVFieldPtr pvField = pvCopy->getSubField(node->structureOffset);
        if(node->isStructure) {
            PVStructurePtr xxx = static_pointer_cast<PVStructure>(pvField);
            CopyStructureNodePtr subStructureNode =
                static_pointer_cast<CopyStructureNode>(node);
            updateStructureNodeFromBitSet(
                 xxx,subStructureNode,bitSet,toCopy,doAll);
        } else {
            CopyRecordNodePtr recordNode =
                static_pointer_cast<CopyRecordNode>(node);
            updateSubFieldFromBitSet(
                pvField,recordNode->recordPVField,bitSet,toCopy,doAll);
        }
    }
}

void PVCopy::updateSubFieldFromBitSet(
    PVFieldPtr const &pvCopy,
    PVRecordFieldPtr const &pvRecordField,
    BitSetPtr const &bitSet,
    bool toCopy,
    bool doAll)
{
    if(!doAll) {
        doAll = bitSet->get(pvCopy->getFieldOffset());
    }
    if(!doAll) {
        size_t offset = pvCopy->getFieldOffset();
        size_t nextSet = bitSet->nextSetBit(offset);
        if(nextSet==String::npos) return;
        if(nextSet>=pvCopy->getNextFieldOffset()) return;
    }
    ConvertPtr convert = getConvert();
    if(pvCopy->getField()->getType()==epics::pvData::structure) {
        PVStructurePtr pvCopyStructure =
            static_pointer_cast<PVStructure>(pvCopy);
        PVFieldPtrArray const & pvCopyFields = pvCopyStructure->getPVFields();
        if(pvRecordField->getPVField()->getField()->getType()
        !=epics::pvData::structure)
        {
            if(pvCopyFields.size()!=1) {
                throw std::logic_error(String("Logic error"));
            }
            if(toCopy) {
                convert->copy(pvRecordField->getPVField(), pvCopyFields[0]);
            } else {
                convert->copy(pvCopyFields[0], pvRecordField->getPVField());
            }
            return;
        }
        PVRecordStructurePtr pvRecordStructure = 
            static_pointer_cast<PVRecordStructure>(pvRecordField);
        PVRecordFieldPtrArrayPtr pvRecordFields =
            pvRecordStructure->getPVRecordFields();
        for(size_t i=0; i<pvCopyFields.size(); i++) {
            updateSubFieldFromBitSet(
                pvCopyFields[i],
                (*pvRecordFields)[i],
                bitSet,toCopy,doAll);
        }
    } else {
        if(toCopy) {
            convert->copy(pvRecordField->getPVField(), pvCopy);
        } else {
            convert->copy(pvCopy, pvRecordField->getPVField());
        }
    }
}

CopyRecordNodePtr PVCopy::getCopyOffset(
        CopyStructureNodePtr const &structureNode,
        PVRecordFieldPtr const &recordPVField)
{
    size_t offset = recordPVField->getPVField()->getFieldOffset();
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i< nodes->size(); i++) {
        CopyNodePtr node = (*nodes)[i];
        if(!node->isStructure) {
            CopyRecordNodePtr recordNode =
                static_pointer_cast<CopyRecordNode>(node);
            size_t off = recordNode->recordPVField->getPVField()->
                getFieldOffset();
            size_t nextOffset = recordNode->recordPVField->getPVField()->
                getNextFieldOffset(); 
            if(offset>= off && offset<nextOffset) return recordNode;
        } else {
            CopyStructureNodePtr subNode =
                static_pointer_cast<CopyStructureNode>(node);
            CopyRecordNodePtr recordNode =
                getCopyOffset(subNode,recordPVField);
            if(recordNode.get()!=NULL) return recordNode;
        }
    }
    return NULLCopyRecordNode;
}

CopyRecordNodePtr PVCopy::getRecordNode(
        CopyStructureNodePtr const &structureNode,
        std::size_t structureOffset)
{
    CopyNodePtrArrayPtr nodes = structureNode->nodes;
    for(size_t i=0; i< nodes->size(); i++) {
        CopyNodePtr node = (*nodes)[i];
        if(structureOffset>=(node->structureOffset + node->nfields)) continue;
        if(!node->isStructure) {
            CopyRecordNodePtr recordNode =
                static_pointer_cast<CopyRecordNode>(node);
            return recordNode;
        }
        CopyStructureNodePtr subNode =
            static_pointer_cast<CopyStructureNode>(node);
        return  getRecordNode(subNode,structureOffset);
    }
    return NULLCopyRecordNode;
}

PVCopyMonitor::PVCopyMonitor(
    PVRecordPtr const &pvRecord,
    CopyNodePtr const &headNode,
    PVCopyPtr const &pvCopy,
    PVCopyMonitorRequesterPtr const &pvCopyMonitorRequester
)
: pvRecord(pvRecord),
  headNode(headNode),
  pvCopy(pvCopy),
  pvCopyMonitorRequester(pvCopyMonitorRequester),
  isGroupPut(false),
  dataChanged(false)
{
}

PVCopyMonitor::~PVCopyMonitor()
{
    pvRecord.reset();
    headNode.reset();
    pvCopy.reset();
    pvCopyMonitorRequester.reset();
    changeBitSet.reset();
    overrunBitSet.reset();
}

void PVCopyMonitor::startMonitoring(
    BitSetPtr const  &changeBitSet,
    BitSetPtr const  &overrunBitSet)
{
    this->changeBitSet = changeBitSet;
    this->overrunBitSet = overrunBitSet;
    isGroupPut = false;
    pvRecord->addListener(getPtrSelf());
    addListener(headNode);
    pvRecord->lock();
    try {
        changeBitSet->clear();
        overrunBitSet->clear();
        changeBitSet->set(0);
        pvCopyMonitorRequester->dataChanged();
        pvRecord->unlock();
    } catch(...) {
        pvRecord->unlock();
    }
}

void PVCopyMonitor::stopMonitoring()
{
    pvRecord->removeListener(getPtrSelf());
}

void PVCopyMonitor::switchBitSets(
    BitSetPtr const &newChangeBitSet,
    BitSetPtr const &newOverrunBitSet,
    bool lockRecord)
{
    if(lockRecord) pvRecord->lock();
    try {
        changeBitSet = newChangeBitSet;
        overrunBitSet = newOverrunBitSet;
        if(lockRecord) pvRecord->unlock();
    } catch(...) {
        if(lockRecord) pvRecord->unlock();
    }
}

void PVCopyMonitor::detach(PVRecordPtr const & pvRecord)
{
}

void PVCopyMonitor::dataPut(PVRecordFieldPtr const & pvRecordField)
{
    CopyNodePtr node = findNode(headNode,pvRecordField);
    if(node.get()==NULL) {
        throw std::logic_error("Logic error");
    }
    size_t offset = node->structureOffset;
    bool isSet = changeBitSet->get(offset);
    changeBitSet->set(offset);
    if(isSet) overrunBitSet->set(offset);
    if(!isGroupPut) pvCopyMonitorRequester->dataChanged();
    dataChanged = true;
}

void PVCopyMonitor::dataPut(
    PVRecordStructurePtr const & requested,
    PVRecordFieldPtr const & pvRecordField)
{
    CopyNodePtr node = findNode(headNode,requested);
    if(node.get()==NULL || node->isStructure) {
        throw std::logic_error("Logic error");
    }
    CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(node);
    size_t offset = recordNode->structureOffset
        + (pvRecordField->getPVField()->getFieldOffset()
             - recordNode->recordPVField->getPVField()->getFieldOffset());
    bool isSet = changeBitSet->get(offset);
    changeBitSet->set(offset);
    if(isSet) overrunBitSet->set(offset);
    if(!isGroupPut) pvCopyMonitorRequester->dataChanged();
    dataChanged = true;
}

void PVCopyMonitor::beginGroupPut(PVRecordPtr const & pvRecord)
{
    isGroupPut = true;
    dataChanged = false;
}

void PVCopyMonitor::endGroupPut(PVRecordPtr const & pvRecord)
{
    isGroupPut = false;
    if(dataChanged) {
         dataChanged = false;
         pvCopyMonitorRequester->dataChanged();
    }
}

void PVCopyMonitor::unlisten(PVRecordPtr const & pvRecord)
{
    pvCopyMonitorRequester->unlisten();
}


void PVCopyMonitor::addListener(CopyNodePtr const & node)
{
    if(!node->isStructure) {
        PVRecordFieldPtr pvRecordField =
             pvCopy->getRecordPVField(node->structureOffset);
        pvRecordField->addListener(getPtrSelf());
        return;
    }
    CopyStructureNodePtr structureNode =
        static_pointer_cast<CopyStructureNode>(node);
    for(size_t i=0; i< structureNode->nodes->size(); i++) {
        addListener((*structureNode->nodes)[i]);
    }
}

CopyNodePtr PVCopyMonitor::findNode(
    CopyNodePtr const & node,
    PVRecordFieldPtr const & pvRecordField)
{
    if(!node->isStructure) {
        CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(node);
        if(recordNode->recordPVField==pvRecordField) return node;
        return NULLCopyNode;
    }
    CopyStructureNodePtr structureNode = static_pointer_cast<CopyStructureNode>(node);
    for(size_t i=0; i<structureNode->nodes->size(); i++) {
        CopyNodePtr xxx = findNode((*structureNode->nodes)[i],pvRecordField);
        if(xxx.get()!=NULL) return xxx;
    }
    return NULLCopyNode;
}

}}
