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

#include <pv/thread.h>

#include <pv/channelProviderLocal.h>
#include <pv/pvCopy.h>


namespace epics { namespace pvDatabase { 

using namespace epics::pvData;
using std::tr1::static_pointer_cast;
using std::tr1::dynamic_pointer_cast;
using std::size_t;
using std::cout;
using std::endl;

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
      nfields(0)
    {}
    bool isStructure;
    size_t structureOffset; // In the copy
    size_t nfields;
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

PVCopy::PVCopy(
    PVRecordPtr const &pvRecord)
: pvRecord(pvRecord)
{
}

void PVCopy::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopy::destroy" << endl;
    }
    headNode.reset();
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

PVStructurePtr PVCopy::getOptions(
    PVStructurePtr const &copyPVStructure,std::size_t fieldOffset)
{
    CopyNodePtr node = headNode;
    while(true) {
        if(!node->isStructure) {
            if(node->structureOffset==fieldOffset) return node->options;
            return NULLPVStructure;
        }
        CopyStructureNodePtr structNode = static_pointer_cast<CopyStructureNode>(node);
        CopyNodePtrArrayPtr nodes = structNode->nodes;
        boolean okToContinue = false;
        for(size_t i=0; i< nodes->size(); i++) {
            node = (*nodes)[i];
            size_t soff = node->structureOffset;
            if(fieldOffset>=soff && fieldOffset<soff+node->nfields) {
                if(fieldOffset==soff) return node->options;
                if(!node->isStructure) {
                    return NULLPVStructure;
                }
                okToContinue = true;
                break;
            }
        }
        if(okToContinue) continue;
        throw std::invalid_argument("fieldOffset not valid");
    }
}

size_t PVCopy::getCopyOffset(PVRecordFieldPtr const &recordPVField)
{
    if(!headNode->isStructure) {
        CopyRecordNodePtr recordNode = static_pointer_cast<CopyRecordNode>(headNode);
        if((recordNode->recordPVField.get())==recordPVField.get()) {
             return headNode->structureOffset;
        }
        PVStructure * parent = recordPVField->getPVField()->getParent();
        size_t offsetParent = parent->getFieldOffset();
        size_t off = recordPVField->getPVField()->getFieldOffset();
        size_t offdiff = off -offsetParent;
        if(offdiff<recordNode->nfields) return headNode->structureOffset + offdiff;
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
    BitSetPtr const  &bitSet)
{
    bitSet->clear();
    bitSet->set(0);
    pvRecord->lock();
    try {
        updateCopyFromBitSet(copyPVStructure,bitSet);
    } catch(...) {
        pvRecord->unlock();
        throw;
    }
    pvRecord->unlock();
}

void PVCopy::updateCopySetBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
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
}

void PVCopy::updateCopyFromBitSet(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
    bool doAll = bitSet->get(0);
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
}

void PVCopy::updateRecord(
    PVStructurePtr const  &copyPVStructure,
    BitSetPtr const  &bitSet)
{
    bool doAll = bitSet->get(0);
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
    if(len==0) entireRecord = true;
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
        recordNode->nfields = pvRecordStructure->getPVStructure()->getNumberFields();
        return true;
    }
    structure = createStructure(pvRecordStructure->getPVStructure(),pvRequest);
    if(structure==NULL) return false;
    cacheInitStructure = createPVStructure();
    headNode = createStructureNodes(
        pvRecord->getPVRecordStructure(),
        pvRequest,
        cacheInitStructure);
    return true;
}

epics::pvData::String PVCopy::dump(
    String const &value,
    CopyNodePtr const &node,
    int indentLevel)
{
    throw std::logic_error(String("Not Implemented"));
}


StructureConstPtr PVCopy::createStructure(
    PVStructurePtr const &pvRecord,
    PVStructurePtr const  &pvFromRequest)
{
    if(pvFromRequest->getStructure()->getNumberFields()==0) {
        return pvRecord->getStructure();
    }
    PVFieldPtrArray const &pvFromRequestFields = pvFromRequest->getPVFields();
    StringArray const &fromRequestFieldNames = pvFromRequest->getStructure()->getFieldNames();
    size_t length = pvFromRequestFields.size();
    if(length==0) return NULLStructure;
    FieldConstPtrArray fields; fields.reserve(length);
    StringArray fieldNames; fields.reserve(length);
    for(size_t i=0; i<length; ++i) {
        String const &fieldName = fromRequestFieldNames[i];
        PVFieldPtr pvRecordField = pvRecord->getSubField(fieldName);
        if(pvRecordField==NULL) continue;
        FieldConstPtr field = pvRecordField->getField();
        if(field->getType()==epics::pvData::structure) {
            PVStructurePtr pvRequestStructure = static_pointer_cast<PVStructure>(
                pvFromRequestFields[i]);
            if(pvRequestStructure->getNumberFields()>0) {
                 StringArray const &names = pvRequestStructure->getStructure()->
                     getFieldNames();
                 size_t num = names.size();
                 if(num>0 && names[0].compare("_options")==0) --num;
                 if(num>0) {
                     if(pvRecordField->getField()->getType()!=epics::pvData::structure) continue;
                     fieldNames.push_back(fieldName);
                     fields.push_back(createStructure(
                         static_pointer_cast<PVStructure>(pvRecordField),
                         pvRequestStructure));
                     continue;
                 }
            }
        }
        fieldNames.push_back(fieldName);
        fields.push_back(field);
    }
    size_t numsubfields = fields.size();
    if(numsubfields==0) return NULLStructure;
    return getFieldCreate()->createStructure(fieldNames, fields);
}

CopyNodePtr PVCopy::createStructureNodes(
    PVRecordStructurePtr const &pvRecordStructure,
    PVStructurePtr const &pvFromRequest,
    PVStructurePtr const &pvFromCopy)
{
    PVFieldPtrArray const & copyPVFields = pvFromCopy->getPVFields();
    PVStructurePtr pvOptions;
    PVFieldPtr pvField = pvFromRequest->getSubField("_options");
    if(pvField!=NULL) pvOptions = static_pointer_cast<PVStructure>(pvField);
    size_t number = copyPVFields.size();
    CopyNodePtrArrayPtr nodes(new CopyNodePtrArray());
    nodes->reserve(number);
    for(size_t i=0; i<number; i++) {
        PVFieldPtr copyPVField = copyPVFields[i];
        String fieldName = copyPVField->getFieldName();
        
        PVStructurePtr requestPVStructure = static_pointer_cast<PVStructure>(
              pvFromRequest->getSubField(fieldName));
        PVStructurePtr pvSubFieldOptions;
        PVFieldPtr pvField = requestPVStructure->getSubField("_options");
        if(pvField!=NULL) pvSubFieldOptions = static_pointer_cast<PVStructure>(pvField);
        PVRecordFieldPtr pvRecordField;
        PVRecordFieldPtrArrayPtr pvRecordFields = pvRecordStructure->getPVRecordFields();
        for(size_t j=0; i<pvRecordFields->size(); j++ ) {
            if((*pvRecordFields)[j]->getPVField()->getFieldName().compare(fieldName)==0) {
                pvRecordField = (*pvRecordFields)[j];
                break;
            }
        }
        size_t numberRequest = requestPVStructure->getPVFields().size();
        if(pvSubFieldOptions!=NULL) numberRequest--;
        if(numberRequest>0) {
            nodes->push_back(createStructureNodes(
                static_pointer_cast<PVRecordStructure>(pvRecordField),
                requestPVStructure,
                static_pointer_cast<PVStructure>(copyPVField)));
            continue;
        }
        CopyRecordNodePtr recordNode(new CopyRecordNode());
        recordNode->options = pvSubFieldOptions;
        recordNode->isStructure = false;
        recordNode->recordPVField = pvRecordField;
        recordNode->nfields = copyPVField->getNumberFields();
        recordNode->structureOffset = copyPVField->getFieldOffset();
        nodes->push_back(recordNode);
    }
    CopyStructureNodePtr structureNode(new CopyStructureNode());
    structureNode->isStructure = true;
    structureNode->nodes = nodes;
    structureNode->structureOffset = pvFromCopy->getFieldOffset();
    structureNode->nfields = pvFromCopy->getNumberFields();
    structureNode->options = pvOptions;
    return structureNode;
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
            updateSubFieldSetBitSet(pvField,recordNode->recordPVField,bitSet);
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
  dataChanged(false),
  isMonitoring(false)
{
}

PVCopyMonitor::~PVCopyMonitor()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "~PVCopyMonitor" << endl;
    }
}

void PVCopyMonitor::destroy()
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::destroy()" << endl;
    }
    stopMonitoring();
    pvCopyMonitorRequester.reset();
    pvCopy.reset();
}
 
void PVCopyMonitor::startMonitoring(
    BitSetPtr const  &changeBitSet,
    BitSetPtr const  &overrunBitSet)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::startMonitoring()" << endl;
    }
    Lock xx(mutex);
    if(isMonitoring) return;
    isMonitoring = true;
    this->changeBitSet = changeBitSet;
    this->overrunBitSet = overrunBitSet;
    isGroupPut = false;
    pvRecord->lock();
    try {
        pvRecord->addListener(getPtrSelf());
        addListener(headNode);
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
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::stopMonitoring()" << endl;
    }
    Lock xx(mutex);
    if(!isMonitoring) return;
    isMonitoring = false;
    pvRecord->removeListener(getPtrSelf());
}

void PVCopyMonitor::switchBitSets(
    BitSetPtr const &newChangeBitSet,
    BitSetPtr const &newOverrunBitSet)
{
    changeBitSet = newChangeBitSet;
    overrunBitSet = newOverrunBitSet;
}

void PVCopyMonitor::detach(PVRecordPtr const & pvRecord)
{
    if(pvRecord->getTraceLevel()>0)
    {
        cout << "PVCopyMonitor::detach()" << endl;
    }
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
