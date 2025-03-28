/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef FTLAbstractHeapRepository_h
#define FTLAbstractHeapRepository_h

#if ENABLE(FTL_JIT)

#include "DFGArrayMode.h"
#include "FTLAbstractHeap.h"
#include "IndexingType.h"

namespace JSC { namespace FTL {

#define FOR_EACH_ABSTRACT_HEAP(macro) \
    macro(typedArrayProperties)

#define FOR_EACH_ABSTRACT_FIELD(macro) \
    macro(ArrayBuffer_data, ArrayBuffer::offsetOfData()) \
    macro(Butterfly_arrayBuffer, Butterfly::offsetOfArrayBuffer()) \
    macro(Butterfly_publicLength, Butterfly::offsetOfPublicLength()) \
    macro(Butterfly_vectorLength, Butterfly::offsetOfVectorLength()) \
    macro(CallFrame_callerFrame, CallFrame::callerFrameOffset()) \
    macro(DirectArguments_callee, DirectArguments::offsetOfCallee()) \
    macro(DirectArguments_length, DirectArguments::offsetOfLength()) \
    macro(DirectArguments_minCapacity, DirectArguments::offsetOfMinCapacity()) \
    macro(DirectArguments_overrides, DirectArguments::offsetOfOverrides()) \
    macro(GetterSetter_getter, GetterSetter::offsetOfGetter()) \
    macro(GetterSetter_setter, GetterSetter::offsetOfSetter()) \
    macro(JSArrayBufferView_length, JSArrayBufferView::offsetOfLength()) \
    macro(JSArrayBufferView_mode, JSArrayBufferView::offsetOfMode()) \
    macro(JSArrayBufferView_vector, JSArrayBufferView::offsetOfVector()) \
    macro(JSCell_cellState, JSCell::cellStateOffset()) \
    macro(JSCell_indexingType, JSCell::indexingTypeOffset()) \
    macro(JSCell_structureID, JSCell::structureIDOffset()) \
    macro(JSCell_typeInfoFlags, JSCell::typeInfoFlagsOffset()) \
    macro(JSCell_typeInfoType, JSCell::typeInfoTypeOffset()) \
    macro(JSCell_usefulBytes, JSCell::indexingTypeOffset()) \
    macro(JSFunction_executable, JSFunction::offsetOfExecutable()) \
    macro(JSFunction_scope, JSFunction::offsetOfScopeChain()) \
    macro(JSFunction_rareData, JSFunction::offsetOfRareData()) \
    macro(JSObject_butterfly, JSObject::butterflyOffset()) \
    macro(JSPropertyNameEnumerator_cachedInlineCapacity, JSPropertyNameEnumerator::cachedInlineCapacityOffset()) \
    macro(JSPropertyNameEnumerator_cachedPropertyNamesVector, JSPropertyNameEnumerator::cachedPropertyNamesVectorOffset()) \
    macro(JSPropertyNameEnumerator_cachedStructureID, JSPropertyNameEnumerator::cachedStructureIDOffset()) \
    macro(JSPropertyNameEnumerator_endGenericPropertyIndex, JSPropertyNameEnumerator::endGenericPropertyIndexOffset()) \
    macro(JSPropertyNameEnumerator_endStructurePropertyIndex, JSPropertyNameEnumerator::endStructurePropertyIndexOffset()) \
    macro(JSPropertyNameEnumerator_indexLength, JSPropertyNameEnumerator::indexedLengthOffset()) \
    macro(JSScope_next, JSScope::offsetOfNext()) \
    macro(JSString_flags, JSString::offsetOfFlags()) \
    macro(JSString_length, JSString::offsetOfLength()) \
    macro(JSString_value, JSString::offsetOfValue()) \
    macro(JSSymbolTableObject_symbolTable, JSSymbolTableObject::offsetOfSymbolTable()) \
    macro(JSWrapperObject_internalValue, JSWrapperObject::internalValueOffset()) \
    macro(MarkedAllocator_freeListHead, MarkedAllocator::offsetOfFreeListHead()) \
    macro(ScopedArguments_overrodeThings, ScopedArguments::offsetOfOverrodeThings()) \
    macro(ScopedArguments_scope, ScopedArguments::offsetOfScope()) \
    macro(ScopedArguments_table, ScopedArguments::offsetOfTable()) \
    macro(ScopedArguments_totalLength, ScopedArguments::offsetOfTotalLength()) \
    macro(ScopedArgumentsTable_arguments, ScopedArgumentsTable::offsetOfArguments()) \
    macro(ScopedArgumentsTable_length, ScopedArgumentsTable::offsetOfLength()) \
    macro(StringImpl_data, StringImpl::dataOffset()) \
    macro(StringImpl_hashAndFlags, StringImpl::flagsOffset()) \
    macro(Structure_classInfo, Structure::classInfoOffset()) \
    macro(Structure_globalObject, Structure::globalObjectOffset()) \
    macro(Structure_prototype, Structure::prototypeOffset()) \
    macro(Structure_structureID, Structure::structureIDOffset()) \
    macro(Symbol_privateName, Symbol::offsetOfPrivateName())

#define FOR_EACH_INDEXED_ABSTRACT_HEAP(macro) \
    macro(DirectArguments_storage, DirectArguments::storageOffset(), sizeof(EncodedJSValue)) \
    macro(JSEnvironmentRecord_variables, JSEnvironmentRecord::offsetOfVariables(), sizeof(EncodedJSValue)) \
    macro(JSPropertyNameEnumerator_cachedPropertyNamesVectorContents, 0, sizeof(WriteBarrier<JSString>)) \
    macro(JSRopeString_fibers, JSRopeString::offsetOfFibers(), sizeof(WriteBarrier<JSString>)) \
    macro(MarkedSpace_Subspace_impreciseAllocators, OBJECT_OFFSETOF(MarkedSpace::Subspace, impreciseAllocators), sizeof(MarkedAllocator)) \
    macro(MarkedSpace_Subspace_preciseAllocators, OBJECT_OFFSETOF(MarkedSpace::Subspace, preciseAllocators), sizeof(MarkedAllocator)) \
    macro(ScopedArguments_overflowStorage, ScopedArguments::overflowStorageOffset(), sizeof(EncodedJSValue)) \
    macro(WriteBarrierBuffer_bufferContents, 0, sizeof(JSCell*)) \
    macro(characters8, 0, sizeof(LChar)) \
    macro(characters16, 0, sizeof(UChar)) \
    macro(indexedInt32Properties, 0, sizeof(EncodedJSValue)) \
    macro(indexedDoubleProperties, 0, sizeof(double)) \
    macro(indexedContiguousProperties, 0, sizeof(EncodedJSValue)) \
    macro(indexedArrayStorageProperties, 0, sizeof(EncodedJSValue)) \
    macro(scopedArgumentsTableArguments, 0, sizeof(int32_t)) \
    macro(singleCharacterStrings, 0, sizeof(JSString*)) \
    macro(structureTable, 0, sizeof(Structure*)) \
    macro(variables, 0, sizeof(Register))
    
#define FOR_EACH_NUMBERED_ABSTRACT_HEAP(macro) \
    macro(properties)
    
// This class is meant to be cacheable between compilations, but it doesn't have to be.
// Doing so saves on creation of nodes. But clearing it will save memory.

class AbstractHeapRepository {
    WTF_MAKE_NONCOPYABLE(AbstractHeapRepository);
public:
    AbstractHeapRepository();
    ~AbstractHeapRepository();
    
    AbstractHeap root;
    
#define ABSTRACT_HEAP_DECLARATION(name) AbstractHeap name;
    FOR_EACH_ABSTRACT_HEAP(ABSTRACT_HEAP_DECLARATION)
#undef ABSTRACT_HEAP_DECLARATION

#define ABSTRACT_FIELD_DECLARATION(name, offset) AbstractField name;
    FOR_EACH_ABSTRACT_FIELD(ABSTRACT_FIELD_DECLARATION)
#undef ABSTRACT_FIELD_DECLARATION
    
    AbstractField& JSCell_freeListNext;
    
#define INDEXED_ABSTRACT_HEAP_DECLARATION(name, offset, size) IndexedAbstractHeap name;
    FOR_EACH_INDEXED_ABSTRACT_HEAP(INDEXED_ABSTRACT_HEAP_DECLARATION)
#undef INDEXED_ABSTRACT_HEAP_DECLARATION
    
#define NUMBERED_ABSTRACT_HEAP_DECLARATION(name) NumberedAbstractHeap name;
    FOR_EACH_NUMBERED_ABSTRACT_HEAP(NUMBERED_ABSTRACT_HEAP_DECLARATION)
#undef NUMBERED_ABSTRACT_HEAP_DECLARATION

    AbsoluteAbstractHeap absolute;
    
    IndexedAbstractHeap* forIndexingType(IndexingType indexingType)
    {
        switch (indexingType) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return 0;
            
        case ALL_INT32_INDEXING_TYPES:
            return &indexedInt32Properties;
            
        case ALL_DOUBLE_INDEXING_TYPES:
            return &indexedDoubleProperties;
            
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return &indexedContiguousProperties;
            
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return &indexedArrayStorageProperties;
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return 0;
        }
    }
    
    IndexedAbstractHeap& forArrayType(DFG::Array::Type type)
    {
        switch (type) {
        case DFG::Array::Int32:
            return indexedInt32Properties;
        case DFG::Array::Double:
            return indexedDoubleProperties;
        case DFG::Array::Contiguous:
            return indexedContiguousProperties;
        case DFG::Array::ArrayStorage:
        case DFG::Array::SlowPutArrayStorage:
            return indexedArrayStorageProperties;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return indexedInt32Properties;
        }
    }

private:
    friend class AbstractHeap;
};

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

#endif // FTLAbstractHeapRepository_h

