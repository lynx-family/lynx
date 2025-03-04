//  Copyright (c) Facebook, Inc. and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsc/jsc_runtime.h"

#include <atomic>
#include <mutex>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/jsi/jsc/jsc_api.h"
#include "core/runtime/jsi/jsc/jsc_context_group_wrapper_impl.h"
#include "core/runtime/jsi/jsc/jsc_context_wrapper_impl.h"
#include "core/runtime/jsi/jsc/jsc_exception.h"
#include "core/runtime/jsi/jsc/jsc_host_function.h"
#include "core/runtime/jsi/jsc/jsc_host_object.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace piper {

using detail::JSCHelper;
using detail::JSCHostFunctionProxy;
using detail::JSCHostObjectProxy;
using detail::JSCObjectValue;
using detail::JSCStringValue;
using detail::JSCSymbolValue;

#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_9_0
#define _JSC_FAST_IS_ARRAY
#endif
#endif

JSCRuntime::JSCRuntime()
    : ctx_group_(nullptr), ctx_(nullptr), temp_ctx_invalid_(true) {}

JSCRuntime::~JSCRuntime() {
  *is_runtime_destroyed_ = true;
  ClearHostContainers();
  ctx_->Release();
  ctx_.reset();
  LOGI("lynx ~JSCRuntime " << ctx_.use_count());
}

bool JSCRuntime::Valid() const { return ctx_ && !(ctx_->contextInvalid()); }

void JSCRuntime::InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                             std::shared_ptr<JSIExceptionHandler> handler) {
  exception_handler_ = handler;
  ctx_group_ =
      std::static_pointer_cast<JSCContextGroupWrapper>(sharedContext->getVM());
  ctx_ = std::static_pointer_cast<JSCContextWrapper>(sharedContext);
  ;
}

std::shared_ptr<VMInstance> JSCRuntime::createVM(const StartupData*) const {
  auto ctx_group = std::make_shared<JSCContextGroupWrapperImpl>();
  ctx_group->InitContextGroup();
  return ctx_group;
}

// TODO
std::shared_ptr<VMInstance> JSCRuntime::getSharedVM() { return ctx_group_; }

std::shared_ptr<JSIContext> JSCRuntime::createContext(
    std::shared_ptr<VMInstance> vm) const {
  auto ctx = std::make_shared<JSCContextWrapperImpl>(vm);
  ctx->init();
  return ctx;
}

std::shared_ptr<JSIContext> JSCRuntime::getSharedContext() { return ctx_; }

std::shared_ptr<const PreparedJavaScript> JSCRuntime::prepareJavaScript(
    const std::shared_ptr<const Buffer>& buffer, std::string sourceURL) {
  return std::make_shared<SourceJavaScriptPreparation>(buffer,
                                                       std::move(sourceURL));
}

base::expected<Value, JSINativeException>
JSCRuntime::evaluatePreparedJavaScript(
    const std::shared_ptr<const PreparedJavaScript>& js) {
  DCHECK(
      // TODO
      static_cast<const SourceJavaScriptPreparation*>(js.get()) &&
      "preparedJavaScript must be a SourceJavaScriptPreparation");
  auto sourceJs =
      std::static_pointer_cast<const SourceJavaScriptPreparation>(js);
  return evaluateJavaScript(sourceJs, sourceJs->sourceURL());
}

base::expected<Value, JSINativeException> JSCRuntime::evaluateJavaScript(
    const std::shared_ptr<const Buffer>& buffer, const std::string& sourceURL) {
  std::string tmp(reinterpret_cast<const char*>(buffer->data()),
                  buffer->size());
  JSStringRef sourceRef = JSStringCreateWithUTF8CString(tmp.c_str());
  JSStringRef sourceURLRef = nullptr;
  if (!sourceURL.empty()) {
    sourceURLRef =
        JSStringCreateWithUTF8CString(("file://" + sourceURL).c_str());
  }
  JSValueRef exc = nullptr;
  JSValueRef res = JSEvaluateScript(ctx_->getContext(), sourceRef, nullptr,
                                    sourceURLRef, 0, &exc);
  JSStringRelease(sourceRef);
  if (sourceURLRef) {
    JSStringRelease(sourceURLRef);
  }
  auto maybe_error =
      JSCException::TryCatch(ctx_->getContext(), *this, res, exc);
  if (maybe_error.has_value()) {
    return base::unexpected(JSINativeException(
        maybe_error->name(), maybe_error->message(), maybe_error->stack(), true,
        error::E_BTS_RUNTIME_ERROR_SCRIPT_ERROR));
  }
  return JSCHelper::createValue(*this, res);
}

Object JSCRuntime::global() {
  return JSCHelper::createObject(getContext(), ctx_->contextInvalid(),
                                 objectCounter(),
                                 JSContextGetGlobalObject(ctx_->getContext()));
}

std::string JSCRuntime::description() {
  if (description_.empty()) {
    std::ostringstream ss;
    ss << this;
    description_ = std::string("<JSCRuntime@") + ss.str() + ">";
  }
  return description_;
}

Runtime::PointerValue* JSCRuntime::cloneSymbol(const PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  const JSCSymbolValue* symbol = static_cast<const JSCSymbolValue*>(pv);
  return JSCHelper::makeSymbolValue(ctx_->getContext(), ctx_->contextInvalid(),
                                    objectCounter(), symbol->sym_);
}

Runtime::PointerValue* JSCRuntime::cloneString(const PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  const JSCStringValue* string = static_cast<const JSCStringValue*>(pv);
  return JSCHelper::makeStringValue(objectCounter(), string->str_);
}

Runtime::PointerValue* JSCRuntime::cloneObject(const PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  const JSCObjectValue* object = static_cast<const JSCObjectValue*>(pv);
  return JSCHelper::makeObjectValue(ctx_->getContext(), ctx_->contextInvalid(),
                                    objectCounter(), object->obj_);
}

Runtime::PointerValue* JSCRuntime::clonePropNameID(const PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  const JSCStringValue* string = static_cast<const JSCStringValue*>(pv);
  return JSCHelper::makeStringValue(objectCounter(), string->str_);
}

PropNameID JSCRuntime::createPropNameIDFromAscii(const char* str,
                                                 size_t length) {
  std::string tmp(str, length);
  JSStringRef strRef = JSStringCreateWithUTF8CString(tmp.c_str());
  auto res = JSCHelper::createPropNameID(objectCounter(), strRef);
  JSStringRelease(strRef);
  return res;
}

PropNameID JSCRuntime::createPropNameIDFromUtf8(const uint8_t* utf8,
                                                size_t length) {
  std::string tmp(reinterpret_cast<const char*>(utf8), length);
  JSStringRef strRef = JSStringCreateWithUTF8CString(tmp.c_str());
  auto res = JSCHelper::createPropNameID(objectCounter(), strRef);
  JSStringRelease(strRef);
  return res;
}

PropNameID JSCRuntime::createPropNameIDFromString(const piper::String& str) {
  return JSCHelper::createPropNameID(objectCounter(),
                                     JSCHelper::stringRef(str));
}

std::string JSCRuntime::utf8(const PropNameID& sym) {
  return JSCHelper::JSStringToSTLString(JSCHelper::stringRef(sym));
}

bool JSCRuntime::compare(const PropNameID& a, const PropNameID& b) {
  return JSStringIsEqual(JSCHelper::stringRef(a), JSCHelper::stringRef(b));
  ;
}

std::optional<std::string> JSCRuntime::symbolToString(const Symbol& sym) {
  auto str = Value(*this, sym).toString(*this);
  if (!str) {
    return std::optional<std::string>();
  }
  return str->utf8(*this);
}

String JSCRuntime::createStringFromAscii(const char* str, size_t length) {
  return this->createStringFromUtf8(reinterpret_cast<const uint8_t*>(str),
                                    length);
}

String JSCRuntime::createStringFromUtf8(const uint8_t* str, size_t length) {
  std::string tmp(reinterpret_cast<const char*>(str), length);
  JSStringRef stringRef = JSStringCreateWithUTF8CString(tmp.c_str());
  auto result = JSCHelper::createString(objectCounter(), stringRef);
  JSStringRelease(stringRef);
  return result;
}

std::string JSCRuntime::utf8(const piper::String& str) {
  return JSCHelper::JSStringToSTLString(JSCHelper::stringRef(str));
}

Object JSCRuntime::createObject() {
  return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                 objectCounter(),
                                 static_cast<JSObjectRef>(nullptr));
}

Object JSCRuntime::createObject(std::shared_ptr<HostObject> ho) {
  return JSCHostObjectProxy::createObject(*this, ctx_->getContext(), ho);
}

std::weak_ptr<HostObject> JSCRuntime::getHostObject(const Object& obj) {
  JSObjectRef object = JSCHelper::objectRef(obj);
  auto metadata =
      static_cast<detail::JSCHostObjectProxy*>(JSObjectGetPrivate(object));
  DCHECK(metadata);
  return metadata->GetHost();
}

HostFunctionType& JSCRuntime::getHostFunction(const Function& obj) {
  JSObjectRef jsc_obj = JSCHelper::objectRef(obj);
  void* data = JSObjectGetPrivate(jsc_obj);
  auto* proxy = static_cast<detail::HostFunctionMetadata*>(data);
  return *proxy->GetHost();
}

std::optional<Value> JSCRuntime::getProperty(const Object& obj,
                                             const piper::String& name) {
  JSObjectRef objRef = JSCHelper::objectRef(obj);
  JSValueRef exc = nullptr;
  JSValueRef result = JSObjectGetProperty(ctx_->getContext(), objRef,
                                          JSCHelper::stringRef(name), &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, result,
                                             exc)) {
    return std::optional<Value>();
  }
  return JSCHelper::createValue(*this, result);
}

std::optional<Value> JSCRuntime::getProperty(const Object& obj,
                                             const PropNameID& name) {
  JSObjectRef objRef = JSCHelper::objectRef(obj);
  JSValueRef exc = nullptr;
  JSValueRef result = JSObjectGetProperty(ctx_->getContext(), objRef,
                                          JSCHelper::stringRef(name), &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, result,
                                             exc)) {
    return std::optional<Value>();
  }
  return JSCHelper::createValue(*this, result);
}

bool JSCRuntime::hasProperty(const Object& obj, const piper::String& name) {
  JSObjectRef objRef = JSCHelper::objectRef(obj);
  return JSObjectHasProperty(ctx_->getContext(), objRef,
                             JSCHelper::stringRef(name));
}

bool JSCRuntime::hasProperty(const Object& obj, const piper::PropNameID& name) {
  JSObjectRef objRef = JSCHelper::objectRef(obj);
  return JSObjectHasProperty(ctx_->getContext(), objRef,
                             JSCHelper::stringRef(name));
}

bool JSCRuntime::setPropertyValue(Object& object, const PropNameID& name,
                                  const Value& value) {
  JSValueRef exc = nullptr;
  JSObjectSetProperty(ctx_->getContext(), JSCHelper::objectRef(object),
                      JSCHelper::stringRef(name),
                      JSCHelper::valueRef(ctx_->getContext(), *this, value),
                      kJSPropertyAttributeNone, &exc);
  return JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, exc);
}

bool JSCRuntime::setPropertyValue(Object& object, const String& name,
                                  const Value& value) {
  JSValueRef exc = nullptr;
  JSObjectSetProperty(ctx_->getContext(), JSCHelper::objectRef(object),
                      JSCHelper::stringRef(name),
                      JSCHelper::valueRef(ctx_->getContext(), *this, value),
                      kJSPropertyAttributeNone, &exc);
  return JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, exc);
}

bool JSCRuntime::isArray(const Object& obj) const {
#if defined(_JSC_FAST_IS_ARRAY)
  if (JSValueIsArray(ctx_->getContext(), JSCHelper::objectRef(obj))) {
    return true;
  }
#endif
  JSObjectRef global = JSContextGetGlobalObject(ctx_->getContext());
  JSStringRef arrayString = JSCHelper::getJSStringFromPool("Array");
  JSValueRef exc = nullptr;
  JSValueRef arrayCtorValue =
      JSObjectGetProperty(ctx_->getContext(), global, arrayString, &exc);
  JSObjectRef arrayCtor =
      JSValueToObject(ctx_->getContext(), arrayCtorValue, &exc);
  JSStringRef isArrayString = JSCHelper::getJSStringFromPool("isArray");
  JSValueRef isArrayValue =
      JSObjectGetProperty(ctx_->getContext(), arrayCtor, isArrayString, &exc);
  JSObjectRef isArray = JSValueToObject(ctx_->getContext(), isArrayValue, &exc);
  JSValueRef arg = JSCHelper::objectRef(obj);
  JSValueRef result = JSObjectCallAsFunction(ctx_->getContext(), isArray,
                                             nullptr, 1, &arg, &exc);
  return JSValueToBoolean(ctx_->getContext(), result);
}

// create BigInt object and and store value with key named "__lynx_val__", then
// add "toString" method to js object
std::optional<BigInt> JSCRuntime::createBigInt(const std::string& value,
                                               Runtime& rt) {
  JSValueRef exc = nullptr;
  JSObjectRef obj = JSObjectMake(ctx_->getContext(), 0, nullptr);
  piper::String big_int_str = String::createFromUtf8(rt, value);
  JSStringRef str = JSCHelper::stringRef(big_int_str);
  JSStringRef big_int_key = JSCHelper::getJSStringFromPool("__lynx_val__");

  // store value with key
  JSObjectSetProperty(ctx_->getContext(), obj, big_int_key,
                      JSValueMakeString(ctx_->getContext(), str), 0, &exc);

  JSStringRef to_string_ref = JSCHelper::getJSStringFromPool("toString");
  const PropNameID prop = PropNameID::forUtf8(rt, "toString");
  // create "toString" function
  Value fun_val = piper::Function::createFromHostFunction(
      rt, prop, 0,
      [value](Runtime& rt, const Value& thisVal, const Value* args,
              size_t count) -> base::expected<Value, JSINativeException> {
        piper::String res = String::createFromUtf8(rt, value);
        return piper::Value(rt, res);
      });

  // add "toString" property to js object as a function
  JSObjectSetProperty(ctx_->getContext(), obj, to_string_ref,
                      JSCHelper::valueRef(ctx_->getContext(), *this, fun_val),
                      kJSPropertyAttributeNone, &exc);
  // add support for "valueOf" and "toJSON"
  JSStringRef value_of_ref = JSCHelper::getJSStringFromPool("valueOf");
  JSObjectSetProperty(ctx_->getContext(), obj, value_of_ref,
                      JSCHelper::valueRef(ctx_->getContext(), *this, fun_val),
                      kJSPropertyAttributeNone, &exc);
  JSStringRef to_json_ref = JSCHelper::getJSStringFromPool("toJSON");
  JSObjectSetProperty(ctx_->getContext(), obj, to_json_ref,
                      JSCHelper::valueRef(ctx_->getContext(), *this, fun_val),
                      kJSPropertyAttributeNone, &exc);

  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, exc)) {
    return std::optional<BigInt>();
  }

  return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                 objectCounter(), obj)
      .getBigInt(*this);
}

bool JSCRuntime::isArrayBuffer(const Object& obj) const {
  if (__builtin_available(iOS 10.0, *)) {
    auto typedArrayType = JSValueGetTypedArrayType(
        ctx_->getContext(), JSCHelper::objectRef(obj), nullptr);
    return typedArrayType == kJSTypedArrayTypeArrayBuffer;
  } else {
    JSObjectRef global = JSContextGetGlobalObject(ctx_->getContext());
    JSObjectRef object_ctor = JSCHelper::getJSObject(
        const_cast<JSCRuntime&>(*this), ctx_->getContext(), global,
        JSCHelper::getJSStringFromPool("Object"));
    if (!object_ctor) {
      return false;
    }
    JSObjectRef prototype = JSCHelper::getJSObject(
        const_cast<JSCRuntime&>(*this), ctx_->getContext(), object_ctor,
        JSCHelper::getJSStringFromPool("prototype"));
    if (!prototype) {
      return false;
    }
    JSObjectRef to_string = JSCHelper::getJSObject(
        const_cast<JSCRuntime&>(*this), ctx_->getContext(), prototype,
        JSCHelper::getJSStringFromPool("toString"));
    if (!prototype) {
      return false;
    }
    JSObjectRef array_buffer = JSCHelper::objectRef(obj);
    JSValueRef exc = nullptr;
    JSValueRef result = JSObjectCallAsFunction(ctx_->getContext(), to_string,
                                               array_buffer, 0, nullptr, &exc);
    JSStringRef result_str =
        JSValueToStringCopy(ctx_->getContext(), result, &exc);
    if (JSStringIsEqualToUTF8CString(result_str, "[object ArrayBuffer]")) {
      JSStringRelease(result_str);
      return true;
    }
    JSStringRelease(result_str);
    return false;
  }
}

uint8_t* JSCRuntime::data(const ArrayBuffer& obj) {
  if (__builtin_available(iOS 10.0, *)) {
    return static_cast<uint8_t*>(JSObjectGetArrayBufferBytesPtr(
        ctx_->getContext(), JSCHelper::objectRef(obj), nullptr));
  } else {
    reportJSIException(JSError(
        *this, "iOS9 should not reach here: JSCRuntime::data(ArrayBuffer&)"));
    return nullptr;
  }
}

// TODO: copyData api is no longer used and can be removed.
size_t JSCRuntime::copyData(const ArrayBuffer& obj, uint8_t* dest_buf,
                            size_t dest_len) {
  if (__builtin_available(iOS 10.0, *)) {
    size_t src_len = obj.length(*this);
    if (dest_len < src_len) {
      return 0;
    }
    void* src_buf = JSObjectGetArrayBufferBytesPtr(
        ctx_->getContext(), JSCHelper::objectRef(obj), nullptr);
    memcpy(dest_buf, src_buf, src_len);
    return src_len;
  } else {
    JSObjectRef global = JSContextGetGlobalObject(ctx_->getContext());
    if (!global) {
      return 0;
    }
    JSObjectRef ab_to_base64 = JSCHelper::getJSObject(
        const_cast<JSCRuntime&>(*this), ctx_->getContext(), global,
        JSCHelper::getJSStringFromPool("__lynxArrayBufferToBase64"));
    if (!ab_to_base64) {
      return 0;
    }
    JSObjectRef array_buffer = JSCHelper::objectRef(obj);
    JSValueRef exc = nullptr;
    JSValueRef base64_value = JSObjectCallAsFunction(
        ctx_->getContext(), ab_to_base64, nullptr, 1, &array_buffer, &exc);
    if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                               base64_value, exc)) {
      return 0;
    }
    JSStringRef base64_str =
        JSValueToStringCopy(ctx_->getContext(), base64_value, &exc);
    if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                               exc)) {
      return 0;
    }
    std::string base64_stl_str = JSCHelper::JSStringToSTLString(base64_str);
    JSStringRelease(base64_str);
    size_t base64_len = base64_stl_str.length();
    if (dest_len < modp_b64_decode_len(base64_len)) {
      return 0;
    }
    size_t decode_len = modp_b64_decode(
        (char*)dest_buf, (const char*)base64_stl_str.c_str(), base64_len);
    return decode_len > 0 ? decode_len : 0;
  }
}

size_t JSCRuntime::size(const ArrayBuffer& obj) {
  if (__builtin_available(iOS 10.0, *)) {
    return JSObjectGetArrayBufferByteLength(ctx_->getContext(),
                                            JSCHelper::objectRef(obj), nullptr);
  } else {
    JSValueRef exc = nullptr;
    JSStringRef byte_length_str = JSCHelper::getJSStringFromPool("byteLength");
    JSObjectRef array_buffer = JSCHelper::objectRef(obj);
    JSValueRef byte_length_value = JSObjectGetProperty(
        ctx_->getContext(), array_buffer, byte_length_str, &exc);
    if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                               exc)) {
      return 0;
    }
    double byte_length =
        JSValueToNumber(ctx_->getContext(), byte_length_value, &exc);
    if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                               exc)) {
      return 0;
    }
    return byte_length < 0 ? 0 : static_cast<size_t>(byte_length);
  }
}

bool JSCRuntime::isFunction(const Object& obj) const {
  return JSObjectIsFunction(ctx_->getContext(), JSCHelper::objectRef(obj));
}

bool JSCRuntime::isHostObject(const Object& obj) const {
  auto cls = JSCHostObjectProxy::getHostObjectClass();
  return cls != nullptr &&
         JSValueIsObjectOfClass(ctx_->getContext(), JSCHelper::objectRef(obj),
                                cls);
}

bool JSCRuntime::isHostFunction(const Function& obj) const {
  auto cls = detail::HostFunctionMetadata::getHostFunctionClass();
  return cls != nullptr &&
         JSValueIsObjectOfClass(ctx_->getContext(), JSCHelper::objectRef(obj),
                                cls);
}

// Very expensive // TODO
std::optional<Array> JSCRuntime::getPropertyNames(const Object& obj) {
  JSPropertyNameArrayRef names =
      JSObjectCopyPropertyNames(ctx_->getContext(), JSCHelper::objectRef(obj));
  size_t len = JSPropertyNameArrayGetCount(names);
  auto result = createArray(len);
  if (!result) {
    return std::optional<Array>();
  }
  for (size_t i = 0; i < len; i++) {
    JSStringRef str = JSPropertyNameArrayGetNameAtIndex(names, i);
    if (!(*result).setValueAtIndex(
            *this, i, JSCHelper::createString(objectCounter(), str))) {
      return std::optional<Array>();
    }
  }
  JSPropertyNameArrayRelease(names);
  return result;
}

std::optional<Array> JSCRuntime::createArray(size_t length) {
  JSValueRef exc = nullptr;
  JSObjectRef obj = JSObjectMakeArray(ctx_->getContext(), 0, nullptr, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, obj,
                                             exc)) {
    return std::optional<Array>();
  }
  JSObjectSetProperty(
      ctx_->getContext(), obj, JSCHelper::getJSStringFromPool("length"),
      JSValueMakeNumber(ctx_->getContext(), static_cast<double>(length)), 0,
      &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, exc)) {
    return std::optional<Array>();
  }
  return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                 objectCounter(), obj)
      .getArray(*this);
}

ArrayBuffer JSCRuntime::createEmptyArrayBuffer() {
  JSValueRef exception = nullptr;
  JSObjectRef obj = nullptr;
  if (__builtin_available(iOS 10.0, *)) {
    JSTypedArrayBytesDeallocator deallocator = [](void* bytes,
                                                  void* deallocatorContext) {
      if (bytes) {
        delete[] static_cast<uint8_t*>(bytes);
      }
    };

    obj = JSObjectMakeArrayBufferWithBytesNoCopy(
        ctx_->getContext(), new uint8_t[]{0}, 0, deallocator, nullptr,
        &exception);
  } else {
    obj = JSCHelper::createArrayBufferFromJS(*this, ctx_->getContext(), {}, 0);
  }

  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                             exception) ||
      !obj) {
    return ArrayBuffer(*this);
  }
  return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                 objectCounter(), obj)
      .getArrayBuffer(*this);
}

ArrayBuffer JSCRuntime::createArrayBufferCopy(const uint8_t* bytes,
                                              size_t byte_length) {
  if (byte_length == 0) {
    return createEmptyArrayBuffer();
  } else if (!bytes) {
    return ArrayBuffer(*this);
  }

  if (__builtin_available(iOS 10.0, *)) {
    JSValueRef exception = nullptr;
    JSTypedArrayBytesDeallocator deallocator = [](void* bytes,
                                                  void* deallocatorContext) {
      if (bytes) {
        free(bytes);
      }
    };
    JSObjectRef obj = nullptr;
    void* dst_buffer = malloc(byte_length);
    if (dst_buffer) {
      memcpy(dst_buffer, bytes, byte_length);
      obj = JSObjectMakeArrayBufferWithBytesNoCopy(
          ctx_->getContext(), dst_buffer, byte_length, deallocator, nullptr,
          &exception);
    }
    if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                               exception) ||
        !obj) {
      return ArrayBuffer(*this);
    }
    return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                   objectCounter(), obj)
        .getArrayBuffer(*this);
  } else {
    JSObjectRef array_buffer = JSCHelper::createArrayBufferFromJS(
        *this, ctx_->getContext(), bytes, byte_length);
    if (!array_buffer) {
      return ArrayBuffer(*this);
    }
    return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                   objectCounter(), array_buffer)
        .getArrayBuffer(*this);
  }
}

ArrayBuffer JSCRuntime::createArrayBufferNoCopy(
    std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) {
  if (byte_length == 0) {
    return createEmptyArrayBuffer();
  } else if (!bytes) {
    return ArrayBuffer(*this);
  }

  if (__builtin_available(iOS 10.0, *)) {
    JSValueRef exception = nullptr;
    JSTypedArrayBytesDeallocator deallocator = [](void* bytes,
                                                  void* deallocatorContext) {
      if (bytes) {
        delete[] static_cast<uint8_t*>(bytes);
      }
    };
    const uint8_t* raw_buffer = bytes.release();
    JSObjectRef obj = JSObjectMakeArrayBufferWithBytesNoCopy(
        ctx_->getContext(), const_cast<uint8_t*>(raw_buffer), byte_length,
        deallocator, nullptr, &exception);
    if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this,
                                               exception) ||
        !obj) {
      return ArrayBuffer(*this);
    }
    return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                   objectCounter(), obj)
        .getArrayBuffer(*this);
  } else {
    JSObjectRef array_buffer = JSCHelper::createArrayBufferFromJS(
        *this, ctx_->getContext(), bytes.get(), byte_length);
    if (!array_buffer) {
      return ArrayBuffer(*this);
    }
    return JSCHelper::createObject(ctx_->getContext(), ctx_->contextInvalid(),
                                   objectCounter(), array_buffer)
        .getArrayBuffer(*this);
  }
}

std::optional<size_t> JSCRuntime::size(const Array& arr) {
  auto size = getProperty(
      arr, JSCHelper::createPropNameID(
               objectCounter(), JSCHelper::getJSStringFromPool("length")));
  if (!size) {
    return std::optional<size_t>();
  }
  return static_cast<size_t>(

      size->getNumber());
}

std::optional<Value> JSCRuntime::getValueAtIndex(const Array& arr, size_t i) {
  JSValueRef exc = nullptr;
  JSValueRef res = JSObjectGetPropertyAtIndex(
      ctx_->getContext(), JSCHelper::objectRef(arr), (int)i, &exc);
  if (!JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, res,
                                             exc)) {
    return std::optional<Value>();
  }
  return JSCHelper::createValue(*this, res);
}

bool JSCRuntime::setValueAtIndexImpl(Array& arr, size_t i, const Value& value) {
  JSValueRef exc = nullptr;
  JSObjectSetPropertyAtIndex(
      ctx_->getContext(), JSCHelper::objectRef(arr), (int)i,
      JSCHelper::valueRef(ctx_->getContext(), *this, value), &exc);
  return JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, exc);
}

Function JSCRuntime::createFunctionFromHostFunction(const PropNameID& name,
                                                    unsigned int paramCount,
                                                    HostFunctionType func) {
  return detail::HostFunctionMetadata::createFunctionFromHostFunction(
      *this, ctx_->getContext(), name, paramCount, std::move(func));
}

std::optional<Value> JSCRuntime::call(const Function& f, const Value& jsThis,
                                      const Value* args, size_t count) {
  return JSCHelper::call(ctx_->getContext(), *this, f, jsThis, args, count);
}

std::optional<Value> JSCRuntime::callAsConstructor(const Function& f,
                                                   const Value* args,
                                                   size_t count) {
  return JSCHelper::callAsConstructor(ctx_->getContext(), *this, f, args,
                                      count);
}

bool JSCRuntime::strictEquals(const Symbol& a, const Symbol& b) const {
  JSValueRef exc = nullptr;
  bool ret = JSValueIsEqual(ctx_->getContext(), JSCHelper::symbolRef(a),
                            JSCHelper::symbolRef(b), &exc);
  JSCException::ReportExceptionIfNeeded(
      const_cast<JSGlobalContextRef>(ctx_->getContext()),
      const_cast<JSCRuntime&>(*this), exc);
  return ret;
}

bool JSCRuntime::strictEquals(const piper::String& a,
                              const piper::String& b) const {
  return JSStringIsEqual(JSCHelper::stringRef(a), JSCHelper::stringRef(b));
}

bool JSCRuntime::strictEquals(const Object& a, const Object& b) const {
  return JSCHelper::objectRef(a) == JSCHelper::objectRef(b);
}

bool JSCRuntime::instanceOf(const Object& o, const Function& f) {
  JSValueRef exc = nullptr;
  bool res = JSValueIsInstanceOfConstructor(ctx_->getContext(),
                                            JSCHelper::objectRef(o),
                                            JSCHelper::objectRef(f), &exc);
  JSCException::ReportExceptionIfNeeded(ctx_->getContext(), *this, exc);
  return res;
}

void JSCRuntime::RequestGC() {
  LOGI("RequestGC");
  if (ctx_ && ctx_->getContext()) {
    JSGarbageCollect(ctx_->getContext());
  }
}

#if LYNX_ENABLE_E2E_TEST
extern "C" void JSSynchronousGarbageCollectForDebugging(JSContextRef ctx);
#endif

void JSCRuntime::RequestGCForTesting() {
  LOGI("RequestGCForTesting");
  if (ctx_ && ctx_->getContext()) {
#if LYNX_ENABLE_E2E_TEST
    JSSynchronousGarbageCollectForDebugging(ctx_->getContext());
#else
    JSGarbageCollect(ctx_->getContext());
#endif
  }
}

std::unique_ptr<Runtime> makeJSCRuntime() {
  return std::make_unique<JSCRuntime>();
}

}  // namespace piper
}  // namespace lynx
