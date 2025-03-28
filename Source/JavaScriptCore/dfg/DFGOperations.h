/*
 * Copyright (C) 2011, 2013-2015 Apple Inc. All rights reserved.
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

#ifndef DFGOperations_h
#define DFGOperations_h

#if ENABLE(DFG_JIT)

#include "JITOperations.h"
#include "PutKind.h"

namespace JSC { namespace DFG {

struct OSRExitBase;

extern "C" {

JSCell* JIT_OPERATION operationStringFromCharCode(ExecState*, int32_t)  WTF_INTERNAL; 
EncodedJSValue JIT_OPERATION operationStringFromCharCodeUntyped(ExecState*, EncodedJSValue)  WTF_INTERNAL;

// These routines are provide callbacks out to C++ implementations of operations too complex to JIT.
JSCell* JIT_OPERATION operationCreateThis(ExecState*, JSObject* constructor, int32_t inlineCapacity) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationToThis(ExecState*, EncodedJSValue encodedOp1) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationToThisStrict(ExecState*, EncodedJSValue encodedOp1) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueBitAnd(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueBitOr(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueBitXor(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueBitLShift(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueBitRShift(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueBitURShift(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueAdd(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueAddNotNumber(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueDiv(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueMul(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationValueSub(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationGetByVal(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationGetByValCell(ExecState*, JSCell*, EncodedJSValue encodedProperty) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationGetByValArrayInt(ExecState*, JSArray*, int32_t) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationGetByValStringInt(ExecState*, JSString*, int32_t) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationToPrimitive(ExecState*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewArray(ExecState*, Structure*, void*, size_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewArrayBuffer(ExecState*, Structure*, size_t, size_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewEmptyArray(ExecState*, Structure*) WTF_INTERNAL;
char* JIT_OPERATION operationNewArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewInt8ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewInt8ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewInt16ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewInt16ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewInt32ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewInt32ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint8ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint8ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint8ClampedArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint8ClampedArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint16ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint16ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint32ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewUint32ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewFloat32ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewFloat32ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
char* JIT_OPERATION operationNewFloat64ArrayWithSize(ExecState*, Structure*, int32_t) WTF_INTERNAL;
char* JIT_OPERATION operationNewFloat64ArrayWithOneArgument(ExecState*, Structure*, EncodedJSValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValNonStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValCellStrict(ExecState*, JSCell*, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValCellNonStrict(ExecState*, JSCell*, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValBeyondArrayBoundsStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValBeyondArrayBoundsNonStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectBeyondArrayBoundsNonStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectNonStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectCellStrict(ExecState*, JSCell*, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectCellNonStrict(ExecState*, JSCell*, EncodedJSValue encodedProperty, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectBeyondArrayBoundsStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutByValDirectBeyondArrayBoundsNonStrict(ExecState*, JSObject*, int32_t index, EncodedJSValue encodedValue) WTF_INTERNAL;
void JIT_OPERATION operationPutDoubleByValBeyondArrayBoundsStrict(ExecState*, JSObject*, int32_t index, double value) WTF_INTERNAL;
void JIT_OPERATION operationPutDoubleByValBeyondArrayBoundsNonStrict(ExecState*, JSObject*, int32_t index, double value) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationArrayPush(ExecState*, EncodedJSValue encodedValue, JSArray*) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationArrayPushDouble(ExecState*, double value, JSArray*) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationArrayPop(ExecState*, JSArray*) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationArrayPopAndRecoverLength(ExecState*, JSArray*) WTF_INTERNAL;
EncodedJSValue JIT_OPERATION operationRegExpExec(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
// These comparisons return a boolean within a size_t such that the value is zero extended to fill the register.
size_t JIT_OPERATION operationRegExpTest(ExecState*, JSCell*, JSCell*) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareStrictEqCell(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
size_t JIT_OPERATION operationCompareStrictEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2) WTF_INTERNAL;
JSCell* JIT_OPERATION operationCreateActivationDirect(ExecState*, Structure*, JSScope*, SymbolTable*, EncodedJSValue);
JSCell* JIT_OPERATION operationCreateDirectArguments(ExecState*, Structure*, int32_t length, int32_t minCapacity);
JSCell* JIT_OPERATION operationCreateDirectArgumentsDuringExit(ExecState*, InlineCallFrame*, JSFunction*, int32_t argumentCount);
JSCell* JIT_OPERATION operationCreateScopedArguments(ExecState*, Structure*, Register* argumentStart, int32_t length, JSFunction* callee, JSLexicalEnvironment*);
JSCell* JIT_OPERATION operationCreateClonedArgumentsDuringExit(ExecState*, InlineCallFrame*, JSFunction*, int32_t argumentCount);
JSCell* JIT_OPERATION operationCreateClonedArguments(ExecState*, Structure*, Register* argumentStart, int32_t length, JSFunction* callee);
void JIT_OPERATION operationCopyRest(ExecState*, JSCell*, Register* argumentStart, unsigned numberOfParamsToSkip, unsigned arraySize);
double JIT_OPERATION operationFModOnInts(int32_t, int32_t) WTF_INTERNAL;
size_t JIT_OPERATION operationObjectIsObject(ExecState*, JSGlobalObject*, JSCell*) WTF_INTERNAL;
size_t JIT_OPERATION operationObjectIsFunction(ExecState*, JSGlobalObject*, JSCell*) WTF_INTERNAL;
JSCell* JIT_OPERATION operationTypeOfObject(ExecState*, JSGlobalObject*, JSCell*) WTF_INTERNAL;
int32_t JIT_OPERATION operationTypeOfObjectAsTypeofType(ExecState*, JSGlobalObject*, JSCell*) WTF_INTERNAL;
char* JIT_OPERATION operationAllocatePropertyStorageWithInitialCapacity(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION operationAllocatePropertyStorage(ExecState*, size_t newSize) WTF_INTERNAL;
char* JIT_OPERATION operationReallocateButterflyToHavePropertyStorageWithInitialCapacity(ExecState*, JSObject*) WTF_INTERNAL;
char* JIT_OPERATION operationReallocateButterflyToGrowPropertyStorage(ExecState*, JSObject*, size_t newSize) WTF_INTERNAL;
char* JIT_OPERATION operationEnsureInt32(ExecState*, JSCell*);
char* JIT_OPERATION operationEnsureDouble(ExecState*, JSCell*);
char* JIT_OPERATION operationEnsureContiguous(ExecState*, JSCell*);
char* JIT_OPERATION operationEnsureArrayStorage(ExecState*, JSCell*);
StringImpl* JIT_OPERATION operationResolveRope(ExecState*, JSString*);
JSString* JIT_OPERATION operationSingleCharacterString(ExecState*, int32_t);

JSCell* JIT_OPERATION operationNewStringObject(ExecState*, JSString*, Structure*);
JSCell* JIT_OPERATION operationToStringOnCell(ExecState*, JSCell*);
JSCell* JIT_OPERATION operationToString(ExecState*, EncodedJSValue);
JSCell* JIT_OPERATION operationCallStringConstructorOnCell(ExecState*, JSCell*);
JSCell* JIT_OPERATION operationCallStringConstructor(ExecState*, EncodedJSValue);
JSCell* JIT_OPERATION operationMakeRope2(ExecState*, JSString*, JSString*);
JSCell* JIT_OPERATION operationMakeRope3(ExecState*, JSString*, JSString*, JSString*);
JSCell* JIT_OPERATION operationStrCat2(ExecState*, EncodedJSValue, EncodedJSValue);
JSCell* JIT_OPERATION operationStrCat3(ExecState*, EncodedJSValue, EncodedJSValue, EncodedJSValue);
char* JIT_OPERATION operationFindSwitchImmTargetForDouble(ExecState*, EncodedJSValue, size_t tableIndex);
char* JIT_OPERATION operationSwitchString(ExecState*, size_t tableIndex, JSString*);
int32_t JIT_OPERATION operationSwitchStringAndGetBranchOffset(ExecState*, size_t tableIndex, JSString*);
char* JIT_OPERATION operationGetButterfly(ExecState*, JSCell*);
char* JIT_OPERATION operationGetArrayBufferVector(ExecState*, JSCell*);
void JIT_OPERATION operationNotifyWrite(ExecState*, WatchpointSet*);
void JIT_OPERATION operationThrowStackOverflowForVarargs(ExecState*) WTF_INTERNAL;
int32_t JIT_OPERATION operationSizeOfVarargs(ExecState*, EncodedJSValue arguments, int32_t firstVarArgOffset);
void JIT_OPERATION operationLoadVarargs(ExecState*, int32_t firstElementDest, EncodedJSValue arguments, int32_t offset, int32_t length, int32_t mandatoryMinimum);

int64_t JIT_OPERATION operationConvertBoxedDoubleToInt52(EncodedJSValue);
int64_t JIT_OPERATION operationConvertDoubleToInt52(double);

void JIT_OPERATION operationProcessTypeProfilerLogDFG(ExecState*) WTF_INTERNAL;

void JIT_OPERATION debugOperationPrintSpeculationFailure(ExecState*, void*, void*) WTF_INTERNAL;

void JIT_OPERATION triggerReoptimizationNow(CodeBlock*, OSRExitBase*) WTF_INTERNAL;

#if USE(JSVALUE32_64)
double JIT_OPERATION operationRandom(JSGlobalObject*);
#endif

#if ENABLE(FTL_JIT)
void JIT_OPERATION triggerTierUpNow(ExecState*) WTF_INTERNAL;
void JIT_OPERATION triggerTierUpNowInLoop(ExecState*) WTF_INTERNAL;
char* JIT_OPERATION triggerOSREntryNow(ExecState*, int32_t bytecodeIndex, int32_t streamIndex) WTF_INTERNAL;
#endif // ENABLE(FTL_JIT)

} // extern "C"

inline P_JITOperation_EStZ operationNewTypedArrayWithSizeForType(TypedArrayType type)
{
    switch (type) {
    case TypeInt8:
        return operationNewInt8ArrayWithSize;
    case TypeInt16:
        return operationNewInt16ArrayWithSize;
    case TypeInt32:
        return operationNewInt32ArrayWithSize;
    case TypeUint8:
        return operationNewUint8ArrayWithSize;
    case TypeUint8Clamped:
        return operationNewUint8ClampedArrayWithSize;
    case TypeUint16:
        return operationNewUint16ArrayWithSize;
    case TypeUint32:
        return operationNewUint32ArrayWithSize;
    case TypeFloat32:
        return operationNewFloat32ArrayWithSize;
    case TypeFloat64:
        return operationNewFloat64ArrayWithSize;
    case NotTypedArray:
    case TypeDataView:
        break;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return 0;
}

inline P_JITOperation_EStJ operationNewTypedArrayWithOneArgumentForType(TypedArrayType type)
{
    switch (type) {
    case TypeInt8:
        return operationNewInt8ArrayWithOneArgument;
    case TypeInt16:
        return operationNewInt16ArrayWithOneArgument;
    case TypeInt32:
        return operationNewInt32ArrayWithOneArgument;
    case TypeUint8:
        return operationNewUint8ArrayWithOneArgument;
    case TypeUint8Clamped:
        return operationNewUint8ClampedArrayWithOneArgument;
    case TypeUint16:
        return operationNewUint16ArrayWithOneArgument;
    case TypeUint32:
        return operationNewUint32ArrayWithOneArgument;
    case TypeFloat32:
        return operationNewFloat32ArrayWithOneArgument;
    case TypeFloat64:
        return operationNewFloat64ArrayWithOneArgument;
    case NotTypedArray:
    case TypeDataView:
        break;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return 0;
}

} } // namespace JSC::DFG

#endif
#endif
