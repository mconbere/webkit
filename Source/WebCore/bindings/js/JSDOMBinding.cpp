/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2013 Michael Pruett <michael@68k.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSDOMBinding.h"

#include "CachedScript.h"
#include "DOMConstructorWithDocument.h"
#include "DOMStringList.h"
#include "ExceptionCode.h"
#include "ExceptionCodeDescription.h"
#include "ExceptionHeaders.h"
#include "ExceptionInterfaces.h"
#include "Frame.h"
#include "HTMLParserIdioms.h"
#include "IDBDatabaseException.h"
#include "JSDOMWindowCustom.h"
#include "JSExceptionBase.h"
#include "SecurityOrigin.h"
#include <inspector/ScriptCallStack.h>
#include <inspector/ScriptCallStackFactory.h>
#include <interpreter/Interpreter.h>
#include <runtime/DateInstance.h>
#include <runtime/Error.h>
#include <runtime/ErrorHandlingScope.h>
#include <runtime/Exception.h>
#include <runtime/ExceptionHelpers.h>
#include <runtime/JSFunction.h>
#include <stdarg.h>
#include <wtf/MathExtras.h>

using namespace JSC;
using namespace Inspector;

namespace WebCore {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(DOMConstructorObject);
STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(DOMConstructorWithDocument);

void addImpureProperty(const AtomicString& propertyName)
{
    JSDOMWindow::commonVM().addImpureProperty(propertyName);
}

JSValue jsStringOrNull(ExecState* exec, const String& s)
{
    if (s.isNull())
        return jsNull();
    return jsStringWithCache(exec, s);
}

JSValue jsOwnedStringOrNull(ExecState* exec, const String& s)
{
    if (s.isNull())
        return jsNull();
    return jsOwnedString(exec, s);
}

JSValue jsStringOrUndefined(ExecState* exec, const String& s)
{
    if (s.isNull())
        return jsUndefined();
    return jsStringWithCache(exec, s);
}

JSValue jsString(ExecState* exec, const URL& url)
{
    return jsStringWithCache(exec, url.string());
}

JSValue jsStringOrNull(ExecState* exec, const URL& url)
{
    if (url.isNull())
        return jsNull();
    return jsStringWithCache(exec, url.string());
}

JSValue jsStringOrUndefined(ExecState* exec, const URL& url)
{
    if (url.isNull())
        return jsUndefined();
    return jsStringWithCache(exec, url.string());
}

String valueToStringWithNullCheck(ExecState* exec, JSValue value)
{
    if (value.isNull())
        return String();
    return value.toString(exec)->value(exec);
}

String valueToStringWithUndefinedOrNullCheck(ExecState* exec, JSValue value)
{
    if (value.isUndefinedOrNull())
        return String();
    return value.toString(exec)->value(exec);
}

JSValue jsDateOrNaN(ExecState* exec, double value)
{
    if (std::isnan(value))
        return jsDoubleNumber(value);
    return jsDateOrNull(exec, value);
}

JSValue jsDateOrNull(ExecState* exec, double value)
{
    if (!std::isfinite(value))
        return jsNull();
    return DateInstance::create(exec->vm(), exec->lexicalGlobalObject()->dateStructure(), value);
}

double valueToDate(ExecState* exec, JSValue value)
{
    if (value.isNumber())
        return value.asNumber();
    if (!value.inherits(DateInstance::info()))
        return std::numeric_limits<double>::quiet_NaN();
    return static_cast<DateInstance*>(value.toObject(exec))->internalNumber();
}

JSC::JSValue jsArray(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<DOMStringList> stringList)
{
    JSC::MarkedArgumentBuffer list;
    if (stringList) {
        for (unsigned i = 0; i < stringList->length(); ++i)
            list.append(jsStringWithCache(exec, stringList->item(i)));
    }
    return JSC::constructArray(exec, 0, globalObject, list);
}

void reportException(ExecState* exec, JSValue exceptionValue, CachedScript* cachedScript)
{
    RELEASE_ASSERT(exec->vm().currentThreadIsHoldingAPILock());
    Exception* exception = jsDynamicCast<Exception*>(exceptionValue);
    if (!exception) {
        exception = exec->lastException();
        if (!exception)
            exception = Exception::create(exec->vm(), exceptionValue, Exception::DoNotCaptureStack);
    }

    reportException(exec, exception, cachedScript);
}

void reportException(ExecState* exec, Exception* exception, CachedScript* cachedScript, ExceptionDetails* exceptionDetails)
{
    RELEASE_ASSERT(exec->vm().currentThreadIsHoldingAPILock());
    if (isTerminatedExecutionException(exception))
        return;

    ErrorHandlingScope errorScope(exec->vm());

    RefPtr<ScriptCallStack> callStack(createScriptCallStackFromException(exec, exception, ScriptCallStack::maxCallStackSizeToCapture));
    exec->clearException();
    exec->clearLastException();

    JSDOMGlobalObject* globalObject = jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
    if (JSDOMWindow* window = jsDynamicCast<JSDOMWindow*>(globalObject)) {
        if (!window->wrapped().isCurrentlyDisplayedInFrame())
            return;
    }

    int lineNumber = 0;
    int columnNumber = 0;
    String exceptionSourceURL;
    if (const ScriptCallFrame* callFrame = callStack->firstNonNativeCallFrame()) {
        lineNumber = callFrame->lineNumber();
        columnNumber = callFrame->columnNumber();
        exceptionSourceURL = callFrame->sourceURL();
    }

    String errorMessage;
    if (ExceptionBase* exceptionBase = toExceptionBase(exception->value()))
        errorMessage = exceptionBase->message() + ": "  + exceptionBase->description();
    else {
        // FIXME: <http://webkit.org/b/115087> Web Inspector: WebCore::reportException should not evaluate JavaScript handling exceptions
        // If this is a custon exception object, call toString on it to try and get a nice string representation for the exception.
        errorMessage = exception->value().toString(exec)->value(exec);

        // We need to clear any new exception that may be thrown in the toString() call above.
        // reportException() is not supposed to be making new exceptions.
        exec->clearException();
        exec->clearLastException();
    }

    ScriptExecutionContext* scriptExecutionContext = globalObject->scriptExecutionContext();
    scriptExecutionContext->reportException(errorMessage, lineNumber, columnNumber, exceptionSourceURL, callStack->size() ? callStack : 0, cachedScript);

    if (exceptionDetails) {
        exceptionDetails->message = errorMessage;
        exceptionDetails->lineNumber = lineNumber;
        exceptionDetails->columnNumber = columnNumber;
        exceptionDetails->sourceURL = exceptionSourceURL;
    }
}

void reportCurrentException(ExecState* exec)
{
    Exception* exception = exec->exception();
    exec->clearException();
    reportException(exec, exception);
}

#define TRY_TO_CREATE_EXCEPTION(interfaceName) \
    case interfaceName##Type: \
        errorObject = toJS(exec, globalObject, interfaceName::create(description)); \
        break;

static JSValue createDOMException(ExecState* exec, ExceptionCode ec, const String* message)
{
    if (!ec)
        return jsUndefined();

    // FIXME: Handle other WebIDL exception types.
    if (ec == TypeError) {
        if (!message || message->isEmpty())
            return createTypeError(exec);
        return createTypeError(exec, *message);
    }

    if (ec == RangeError) {
        if (!message || message->isEmpty())
            return createRangeError(exec, ASCIILiteral("Bad value"));
        return createRangeError(exec, *message);
    }

    // FIXME: All callers to setDOMException need to pass in the right global object
    // for now, we're going to assume the lexicalGlobalObject. Which is wrong in cases like this:
    // frames[0].document.createElement(null, null); // throws an exception which should have the subframes prototypes.
    JSDOMGlobalObject* globalObject = deprecatedGlobalObjectForPrototype(exec);

    ExceptionCodeDescription description(ec);

    CString messageCString;
    if (message)
        messageCString = message->utf8();
    if (message && !message->isEmpty()) {
        // It is safe to do this because the char* contents of the CString are copied into a new WTF::String before the CString is destroyed.
        description.description = messageCString.data();
    }

    JSValue errorObject;
    switch (description.type) {
        DOM_EXCEPTION_INTERFACES_FOR_EACH(TRY_TO_CREATE_EXCEPTION)
#if ENABLE(INDEXED_DATABASE)
    case IDBDatabaseExceptionType:
        errorObject = toJS(exec, globalObject, DOMCoreException::createWithDescriptionAsMessage(description));
#endif
    }
    
    ASSERT(errorObject);
    addErrorInfo(exec, asObject(errorObject), true);
    return errorObject;
}

static JSValue createDOMException(ExecState* exec, ExceptionCode ec, const String& message)
{
    return createDOMException(exec, ec, &message);
}

JSValue createDOMException(ExecState* exec, ExceptionCode ec)
{
    return createDOMException(exec, ec, nullptr);
}

void setDOMException(ExecState* exec, ExceptionCode ec)
{
    if (!ec || exec->hadException())
        return;

    exec->vm().throwException(exec, createDOMException(exec, ec));
}

void setDOMException(JSC::ExecState* exec, const ExceptionCodeWithMessage& ec)
{
    if (!ec.code || exec->hadException())
        return;

    exec->vm().throwException(exec, createDOMException(exec, ec.code, ec.message));
}

#undef TRY_TO_CREATE_EXCEPTION

bool shouldAllowAccessToNode(ExecState* exec, Node* node)
{
    return BindingSecurity::shouldAllowAccessToNode(exec, node);
}

bool shouldAllowAccessToFrame(ExecState* exec, Frame* target)
{
    return BindingSecurity::shouldAllowAccessToFrame(exec, target);
}

bool shouldAllowAccessToFrame(ExecState* exec, Frame* frame, String& message)
{
    if (!frame)
        return false;
    if (BindingSecurity::shouldAllowAccessToFrame(exec, frame, DoNotReportSecurityError))
        return true;
    message = frame->document()->domWindow()->crossDomainAccessErrorMessage(activeDOMWindow(exec));
    return false;
}

bool shouldAllowAccessToDOMWindow(ExecState* exec, DOMWindow& target, String& message)
{
    if (BindingSecurity::shouldAllowAccessToDOMWindow(exec, target, DoNotReportSecurityError))
        return true;
    message = target.crossDomainAccessErrorMessage(activeDOMWindow(exec));
    return false;
}

void printErrorMessageForFrame(Frame* frame, const String& message)
{
    if (!frame)
        return;
    frame->document()->domWindow()->printErrorMessage(message);
}

Structure* getCachedDOMStructure(JSDOMGlobalObject& globalObject, const ClassInfo* classInfo)
{
    JSDOMStructureMap& structures = globalObject.structures();
    return structures.get(classInfo).get();
}

Structure* cacheDOMStructure(JSDOMGlobalObject& globalObject, Structure* structure, const ClassInfo* classInfo)
{
    JSDOMStructureMap& structures = globalObject.structures();
    ASSERT(!structures.contains(classInfo));
    return structures.set(classInfo, WriteBarrier<Structure>(globalObject.vm(), &globalObject, structure)).iterator->value.get();
}

static const int32_t kMaxInt32 = 0x7fffffff;
static const int32_t kMinInt32 = -kMaxInt32 - 1;
static const uint32_t kMaxUInt32 = 0xffffffffU;
static const int64_t kJSMaxInteger = 0x20000000000000LL - 1; // 2^53 - 1, largest integer exactly representable in ECMAScript.

static String rangeErrorString(double value, double min, double max)
{
    return makeString("Value ", String::numberToStringECMAScript(value), " is outside the range [", String::numberToStringECMAScript(min), ", ", String::numberToStringECMAScript(max), "]");
}

static double enforceRange(ExecState* exec, double x, double minimum, double maximum)
{
    if (std::isnan(x) || std::isinf(x)) {
        exec->vm().throwException(exec, createTypeError(exec, rangeErrorString(x, minimum, maximum)));
        return 0;
    }
    x = trunc(x);
    if (x < minimum || x > maximum) {
        exec->vm().throwException(exec, createTypeError(exec, rangeErrorString(x, minimum, maximum)));
        return 0;
    }
    return x;
}

template <typename T>
struct IntTypeLimits {
};

template <>
struct IntTypeLimits<int8_t> {
    static const int8_t minValue = -128;
    static const int8_t maxValue = 127;
    static const unsigned numberOfValues = 256; // 2^8
};

template <>
struct IntTypeLimits<uint8_t> {
    static const uint8_t maxValue = 255;
    static const unsigned numberOfValues = 256; // 2^8
};

template <>
struct IntTypeLimits<int16_t> {
    static const short minValue = -32768;
    static const short maxValue = 32767;
    static const unsigned numberOfValues = 65536; // 2^16
};

template <>
struct IntTypeLimits<uint16_t> {
    static const unsigned short maxValue = 65535;
    static const unsigned numberOfValues = 65536; // 2^16
};

template <typename T>
static inline T toSmallerInt(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    typedef IntTypeLimits<T> LimitsTrait;
    // Fast path if the value is already a 32-bit signed integer in the right range.
    if (value.isInt32()) {
        int32_t d = value.asInt32();
        if (d >= LimitsTrait::minValue && d <= LimitsTrait::maxValue)
            return static_cast<T>(d);
        if (configuration == EnforceRange) {
            throwTypeError(exec);
            return 0;
        }
        d %= LimitsTrait::numberOfValues;
        return static_cast<T>(d > LimitsTrait::maxValue ? d - LimitsTrait::numberOfValues : d);
    }

    double x = value.toNumber(exec);
    if (exec->hadException())
        return 0;

    if (configuration == EnforceRange)
        return enforceRange(exec, x, LimitsTrait::minValue, LimitsTrait::maxValue);

    if (std::isnan(x) || std::isinf(x) || !x)
        return 0;

    x = x < 0 ? -floor(fabs(x)) : floor(fabs(x));
    x = fmod(x, LimitsTrait::numberOfValues);

    return static_cast<T>(x > LimitsTrait::maxValue ? x - LimitsTrait::numberOfValues : x);
}

template <typename T>
static inline T toSmallerUInt(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    typedef IntTypeLimits<T> LimitsTrait;
    // Fast path if the value is already a 32-bit unsigned integer in the right range.
    if (value.isUInt32()) {
        uint32_t d = value.asUInt32();
        if (d <= LimitsTrait::maxValue)
            return static_cast<T>(d);
        if (configuration == EnforceRange) {
            throwTypeError(exec);
            return 0;
        }
        return static_cast<T>(d);
    }

    double x = value.toNumber(exec);
    if (exec->hadException())
        return 0;

    if (configuration == EnforceRange)
        return enforceRange(exec, x, 0, LimitsTrait::maxValue);

    if (std::isnan(x) || std::isinf(x) || !x)
        return 0;

    x = x < 0 ? -floor(fabs(x)) : floor(fabs(x));
    return static_cast<T>(fmod(x, LimitsTrait::numberOfValues));
}

// http://www.w3.org/TR/WebIDL/#es-byte
int8_t toInt8(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    return toSmallerInt<int8_t>(exec, value, configuration);
}

// http://www.w3.org/TR/WebIDL/#es-octet
uint8_t toUInt8(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    return toSmallerUInt<uint8_t>(exec, value, configuration);
}

// http://www.w3.org/TR/WebIDL/#es-short
int16_t toInt16(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    return toSmallerInt<int16_t>(exec, value, configuration);
}

// http://www.w3.org/TR/WebIDL/#es-unsigned-short
uint16_t toUInt16(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    return toSmallerUInt<uint16_t>(exec, value, configuration);
}

// http://www.w3.org/TR/WebIDL/#es-long
int32_t toInt32EnforceRange(ExecState* exec, JSValue value)
{
    if (value.isInt32())
        return value.asInt32();

    double x = value.toNumber(exec);
    if (exec->hadException())
        return 0;
    return enforceRange(exec, x, kMinInt32, kMaxInt32);
}

// http://www.w3.org/TR/WebIDL/#es-unsigned-long
uint32_t toUInt32EnforceRange(ExecState* exec, JSValue value)
{
    if (value.isUInt32())
        return value.asUInt32();

    double x = value.toNumber(exec);
    if (exec->hadException())
        return 0;
    return enforceRange(exec, x, 0, kMaxUInt32);
}

// http://www.w3.org/TR/WebIDL/#es-long-long
int64_t toInt64(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    if (value.isInt32())
        return value.asInt32();

    double x = value.toNumber(exec);
    if (exec->hadException())
        return 0;

    if (configuration == EnforceRange)
        return enforceRange(exec, x, -kJSMaxInteger, kJSMaxInteger);

    // Map NaNs and +/-Infinity to 0; convert finite values modulo 2^64.
    unsigned long long n;
    doubleToInteger(x, n);
    return n;
}

// http://www.w3.org/TR/WebIDL/#es-unsigned-long-long
uint64_t toUInt64(ExecState* exec, JSValue value, IntegerConversionConfiguration configuration)
{
    if (value.isUInt32())
        return value.asUInt32();

    double x = value.toNumber(exec);
    if (exec->hadException())
        return 0;

    if (configuration == EnforceRange)
        return enforceRange(exec, x, 0, kJSMaxInteger);

    // Map NaNs and +/-Infinity to 0; convert finite values modulo 2^64.
    unsigned long long n;
    doubleToInteger(x, n);
    return n;
}

DOMWindow& activeDOMWindow(ExecState* exec)
{
    return asJSDOMWindow(exec->lexicalGlobalObject())->wrapped();
}

DOMWindow& firstDOMWindow(ExecState* exec)
{
    return asJSDOMWindow(exec->vmEntryGlobalObject())->wrapped();
}

static inline bool canAccessDocument(JSC::ExecState* state, Document* targetDocument, SecurityReportingOption reportingOption = ReportSecurityError)
{
    if (!targetDocument)
        return false;

    DOMWindow& active = activeDOMWindow(state);

    if (active.document()->securityOrigin()->canAccess(targetDocument->securityOrigin()))
        return true;

    if (reportingOption == ReportSecurityError)
        printErrorMessageForFrame(targetDocument->frame(), targetDocument->domWindow()->crossDomainAccessErrorMessage(active));

    return false;
}

bool BindingSecurity::shouldAllowAccessToDOMWindow(JSC::ExecState* state, DOMWindow& target, SecurityReportingOption reportingOption)
{
    return canAccessDocument(state, target.document(), reportingOption);
}

bool BindingSecurity::shouldAllowAccessToFrame(JSC::ExecState* state, Frame* target, SecurityReportingOption reportingOption)
{
    return target && canAccessDocument(state, target->document(), reportingOption);
}

bool BindingSecurity::shouldAllowAccessToNode(JSC::ExecState* state, Node* target)
{
    return target && canAccessDocument(state, &target->document());
}
    
static EncodedJSValue throwTypeError(JSC::ExecState& state, const String& errorMessage)
{
    return throwVMError(&state, createTypeError(&state, errorMessage));
}

static void appendArgumentMustBe(StringBuilder& builder, unsigned argumentIndex, const char* argumentName, const char* interfaceName, const char* functionName)
{
    builder.appendLiteral("Argument ");
    builder.appendNumber(argumentIndex + 1);
    builder.appendLiteral(" ('");
    builder.append(argumentName);
    builder.appendLiteral("') to ");
    if (!functionName) {
        builder.appendLiteral("the ");
        builder.append(interfaceName);
        builder.appendLiteral(" constructor");
    } else {
        builder.append(interfaceName);
        builder.append('.');
        builder.append(functionName);
    }
    builder.appendLiteral(" must be ");
}

JSC::EncodedJSValue reportDeprecatedGetterError(JSC::ExecState& state, const char* interfaceName, const char* attributeName)
{
    auto& context = *jsCast<JSDOMGlobalObject*>(state.lexicalGlobalObject())->scriptExecutionContext();
    context.addConsoleMessage(MessageSource::JS, MessageLevel::Error, makeString("Deprecated attempt to access property '", attributeName, "' on a non-", interfaceName, " object."));
    return JSValue::encode(jsUndefined());
}
    
void reportDeprecatedSetterError(JSC::ExecState& state, const char* interfaceName, const char* attributeName)
{
    auto& context = *jsCast<JSDOMGlobalObject*>(state.lexicalGlobalObject())->scriptExecutionContext();
    context.addConsoleMessage(MessageSource::JS, MessageLevel::Error, makeString("Deprecated attempt to set property '", attributeName, "' on a non-", interfaceName, " object."));
}

JSC::EncodedJSValue throwArgumentMustBeEnumError(JSC::ExecState& state, unsigned argumentIndex, const char* argumentName, const char* functionInterfaceName, const char* functionName, const char* expectedValues)
{
    StringBuilder builder;
    appendArgumentMustBe(builder, argumentIndex, argumentName, functionInterfaceName, functionName);
    builder.appendLiteral("one of: ");
    builder.append(expectedValues);
    return throwTypeError(state, builder.toString());
}

JSC::EncodedJSValue throwArgumentMustBeFunctionError(JSC::ExecState& state, unsigned argumentIndex, const char* argumentName, const char* interfaceName, const char* functionName)
{
    StringBuilder builder;
    appendArgumentMustBe(builder, argumentIndex, argumentName, interfaceName, functionName);
    builder.appendLiteral("a function");
    return throwTypeError(state, builder.toString());
}

JSC::EncodedJSValue throwArgumentTypeError(JSC::ExecState& state, unsigned argumentIndex, const char* argumentName, const char* functionInterfaceName, const char* functionName, const char* expectedType)
{
    StringBuilder builder;
    appendArgumentMustBe(builder, argumentIndex, argumentName, functionInterfaceName, functionName);
    builder.appendLiteral("an instance of ");
    builder.append(expectedType);
    return throwTypeError(state, builder.toString());
}

void throwArrayElementTypeError(JSC::ExecState& state)
{
    throwTypeError(state, "Invalid Array element type");
}

void throwAttributeTypeError(JSC::ExecState& state, const char* interfaceName, const char* attributeName, const char* expectedType)
{
    throwTypeError(state, makeString("The ", interfaceName, '.', attributeName, " attribute must be an instance of ", expectedType));
}

JSC::EncodedJSValue throwConstructorDocumentUnavailableError(JSC::ExecState& state, const char* interfaceName)
{
    // FIXME: This is confusing exception wording. Can we reword to be clearer and more specific?
    return throwVMError(&state, createReferenceError(&state, makeString(interfaceName, " constructor associated document is unavailable")));
}

JSC::EncodedJSValue throwGetterTypeError(JSC::ExecState& state, const char* interfaceName, const char* attributeName)
{
    return throwTypeError(state, makeString("The ", interfaceName, '.', attributeName, " getter can only be used on instances of ", interfaceName));
}

void throwSequenceTypeError(JSC::ExecState& state)
{
    throwTypeError(state, "Value is not a sequence");
}

void throwSetterTypeError(JSC::ExecState& state, const char* interfaceName, const char* attributeName)
{
    throwTypeError(state, makeString("The ", interfaceName, '.', attributeName, " setter can only be used on instances of ", interfaceName));
}

EncodedJSValue throwThisTypeError(JSC::ExecState& state, const char* interfaceName, const char* functionName)
{
    return throwTypeError(state, makeString("Can only call ", interfaceName, '.', functionName, " on instances of ", interfaceName));
}

void callFunctionWithCurrentArguments(JSC::ExecState& state, JSC::JSObject& thisObject, JSC::JSFunction& function)
{
    JSC::CallData callData;
    JSC::CallType callType = JSC::getCallData(&function, callData);
    ASSERT(callType != CallTypeNone);

    JSC::MarkedArgumentBuffer arguments;
    for (unsigned i = 0; i < state.argumentCount(); ++i)
        arguments.append(state.uncheckedArgument(i));
    JSC::call(&state, &function, callType, callData, &thisObject, arguments);
}

void DOMConstructorJSBuiltinObject::visitChildren(JSC::JSCell* cell, JSC::SlotVisitor& visitor)
{
    auto* thisObject = jsCast<DOMConstructorJSBuiltinObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_initializeFunction);
}

static EncodedJSValue JSC_HOST_CALL callThrowTypeError(ExecState* exec)
{
    throwTypeError(exec, ASCIILiteral("Constructor requires 'new' operator"));
    return JSValue::encode(jsNull());
}

CallType DOMConstructorObject::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callThrowTypeError;
    return CallTypeHost;
}

} // namespace WebCore
