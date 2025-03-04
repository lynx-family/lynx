// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_runtime.h"

#include <memory>
#include <mutex>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/common/args_converter.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/v8/v8_context_wrapper_impl.h"
#include "core/runtime/jsi/v8/v8_exception.h"
#include "core/runtime/jsi/v8/v8_helper.h"
#include "core/runtime/jsi/v8/v8_host_function.h"
#include "core/runtime/jsi/v8/v8_host_object.h"
#include "core/runtime/jsi/v8/v8_isolate_wrapper_impl.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper_impl.h"
#include "libplatform/libplatform.h"
#if V8_MAJOR_VERSION >= 9
#include "v8-proxy.h"
#endif
#include "v8.h"

#if defined(OS_ANDROID)
#include "core/runtime/jscache/v8/v8_cache_generator.h"
#endif

namespace lynx {
namespace piper {

#if defined(OS_ANDROID)
namespace cache {
extern std::shared_ptr<Buffer> TryGetCacheV8(
    const std::string& source_url, const std::string& template_url,
    int64_t runtime_id, const std::shared_ptr<const Buffer>& buffer,
    std::unique_ptr<CacheGenerator> cache_generator);

extern void RequestCacheGenerationV8(
    const std::string& source_url, const std::string& template_url,
    const std::shared_ptr<const Buffer>& buffer,
    std::unique_ptr<CacheGenerator> cache_generator, bool force);
}  // namespace cache
#endif

using detail::V8Helper;
using detail::V8HostFunctionProxy;
using detail::V8HostObjectProxy;

V8Runtime::V8Runtime() : isolate_wrapper_(nullptr), ctxInvalid_(true) {}

V8Runtime::~V8Runtime() {
  host_object_template_.Reset();
  host_function_template_.Reset();
  *is_runtime_destroyed_ = true;
  ClearHostContainers();
  ctxInvalid_ = true;
  // ctx_.Reset();
  Finalize();

  context_->Release();
  context_.reset();
}

void V8Runtime::InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                            std::shared_ptr<JSIExceptionHandler> handler) {
  exception_handler_ = handler;
  isolate_wrapper_ =
      std::static_pointer_cast<V8IsolateInstance>(sharedContext->getVM());
  context_ = std::static_pointer_cast<V8ContextWrapper>(sharedContext);
}

std::shared_ptr<VMInstance> V8Runtime::createVM(const StartupData* data) const {
  //  const V8StartupData* v8Data = (const V8StartupData*)data;
  //  return CreateVM_(v8Data->args().c_str(), false);
  return CreateVM_(nullptr, false);
}

std::shared_ptr<VMInstance> V8Runtime::getSharedVM() {
  return isolate_wrapper_;
}

std::shared_ptr<JSIContext> V8Runtime::createContext(
    std::shared_ptr<VMInstance> vm) const {
  return CreateContext_(vm);
}

std::shared_ptr<JSIContext> V8Runtime::getSharedContext() { return context_; }

std::shared_ptr<const PreparedJavaScript> V8Runtime::prepareJavaScript(
    const std::shared_ptr<const Buffer>& buffer, std::string sourceURL) {
  return std::make_shared<piper::SourceJavaScriptPreparation>(
      buffer, std::move(sourceURL));
}

base::expected<Value, JSINativeException> V8Runtime::evaluatePreparedJavaScript(
    const std::shared_ptr<const PreparedJavaScript>& js) {
  const SourceJavaScriptPreparation* source =
      static_cast<const SourceJavaScriptPreparation*>(js.get());
  return evaluateJavaScript(source->buffer(), source->sourceURL());
}

// TODO(zhenziqi): refactor this method
base::expected<Value, JSINativeException> V8Runtime::evaluateJavaScript(
    const std::shared_ptr<const Buffer>& buffer, const std::string& sourceURL) {
  LOGI("V8Runtime::evaluateJavaScript start url=" << sourceURL);
  v8::Isolate* isolate_ = getIsolate();
  v8::Local<v8::Context> context = getContext();
  v8::TryCatch tc(isolate_);
  auto str = v8::String::NewFromUtf8(
      isolate_, reinterpret_cast<const char*>(buffer->data()),
      v8::NewStringType::kNormal, static_cast<int>(buffer->size()));

  std::string origin_url = sourceURL;
  if (inspector_manager_ != nullptr) {
    origin_url = inspector_manager_->BuildInspectorUrl(origin_url);
    inspector_manager_->PrepareForScriptEval();
  }

  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, "evaluatePreparedJavaScript", "url",
                      origin_url, "runtime_id", getRuntimeId());
#if V8_MAJOR_VERSION >= 9
  auto origin = std::make_unique<v8::ScriptOrigin>(
      isolate_, V8Helper::ConvertToV8String(isolate_, origin_url));
#else
  auto origin = std::make_unique<v8::ScriptOrigin>(
      V8Helper::ConvertToV8String(isolate_, origin_url));
#endif
  v8::MaybeLocal<v8::Script> maybe_local_script;
#if defined(OS_ANDROID)
  if (enable_user_bytecode_) {
    LOGI("using v8 bytecode");
    auto generator =
        std::make_unique<cache::V8CacheGenerator>(origin_url, buffer);
    auto cache =
        cache::TryGetCacheV8(sourceURL, bytecode_source_url_, getRuntimeId(),
                             buffer, std::move(generator));
    if (cache) {
      auto* cached_data =
          new v8::ScriptCompiler::CachedData(cache->data(), cache->size());
      // Here ownership of cached_data is transferred to src.
      v8::ScriptCompiler::Source src(str.ToLocalChecked(), *origin,
                                     cached_data);
      maybe_local_script = v8::ScriptCompiler::Compile(
          context, &src, v8::ScriptCompiler::kConsumeCodeCache);
      if (cached_data->rejected) {
        // regenerate cache file
        cache::RequestCacheGenerationV8(
            sourceURL, bytecode_source_url_, buffer,
            std::make_unique<cache::V8CacheGenerator>(origin_url, buffer),
            true);
      }
    }
  } else {
    LOGI("v8 bytecode is disabled in this LynxView");
  }
#endif

  if (maybe_local_script.IsEmpty()) {
    maybe_local_script =
        v8::Script::Compile(context, str.ToLocalChecked(), origin.get());
  }
  v8::Local<v8::Script> script;
  if (!maybe_local_script.IsEmpty()) {
    script = maybe_local_script.ToLocalChecked();
  }
  auto maybe_error = V8Exception::TryCatch(*this, tc);
  if (maybe_error.has_value()) {
    return base::unexpected(JSINativeException(
        maybe_error->name(), maybe_error->message(), maybe_error->stack(), true,
        error::E_BTS_RUNTIME_ERROR_SCRIPT_ERROR));
  }

  if (!script.IsEmpty()) {
    auto result = script->Run(context);

    maybe_error = V8Exception::TryCatch(*this, tc);
    if (maybe_error.has_value()) {
      return base::unexpected(JSINativeException(
          maybe_error->name(), maybe_error->message(), maybe_error->stack(),
          true, error::E_BTS_RUNTIME_ERROR_SCRIPT_ERROR));
    }

    piper::Value value =
        V8Helper::createValue(result.ToLocalChecked(), context);
    return value;
  }
  return Value::undefined();
}

Object V8Runtime::global() {
  return V8Helper::createObject(getContext()->Global(), getIsolate());
}

std::shared_ptr<V8IsolateInstance> V8Runtime::CreateVM_(
    const char* arg, bool useSnapshot) const {
  auto isolate_wrapper = std::make_shared<V8IsolateInstanceImpl>();
  isolate_wrapper->InitIsolate(arg, useSnapshot);
  return isolate_wrapper;
}

std::shared_ptr<V8ContextWrapper> V8Runtime::CreateContext_(
    std::shared_ptr<VMInstance> vm) const {
  auto context_wrapper = std::make_shared<V8ContextWrapperImpl>(vm);
  context_wrapper->Init();
  return context_wrapper;
}

////////////////////////
// protected
////////////////////////

Runtime::PointerValue* V8Runtime::cloneSymbol(const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  v8::Isolate* isolate_ = getIsolate();

  const detail::V8SymbolValue* symbol =
      static_cast<const detail::V8SymbolValue*>(pv);
  return V8Helper::makeSymbolValue(symbol->sym_.Get(isolate_), isolate_);
}

Runtime::PointerValue* V8Runtime::cloneString(const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  v8::Isolate* isolate_ = getIsolate();

  const detail::V8StringValue* string =
      static_cast<const detail::V8StringValue*>(pv);
  return V8Helper::makeStringValue(string->Get(), isolate_);
}

Runtime::PointerValue* V8Runtime::cloneObject(const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  v8::Isolate* isolate_ = getIsolate();

  const detail::V8ObjectValue* object =
      static_cast<const detail::V8ObjectValue*>(pv);
  DCHECK(object->iso_ == isolate_ &&
         "Don't try to clone an object backed by a different Runtime");
  return V8Helper::makeObjectValue(object->Get(), isolate_);
}

Runtime::PointerValue* V8Runtime::clonePropNameID(
    const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }
  v8::Isolate* isolate_ = getIsolate();

  const detail::V8StringValue* string =
      static_cast<const detail::V8StringValue*>(pv);
  return V8Helper::makeStringValue(string->Get(), isolate_);
}

piper::PropNameID V8Runtime::createPropNameIDFromAscii(const char* str,
                                                       size_t length) {
  // For system JSC this must is identical to a string
  v8::Local<v8::String> strRef =
      v8::String::NewFromUtf8(getIsolate(), str, v8::NewStringType::kNormal,
                              static_cast<int>(length))
          .ToLocalChecked();
  auto res = V8Helper::createPropNameID(strRef, getIsolate());
  // JSStringRelease(strRef);
  return res;
}

piper::PropNameID V8Runtime::createPropNameIDFromUtf8(const uint8_t* utf8,
                                                      size_t length) {
  v8::Local<v8::String> strRef =
      v8::String::NewFromUtf8(getIsolate(), reinterpret_cast<const char*>(utf8),
                              v8::NewStringType::kNormal,
                              static_cast<int>(length))
          .ToLocalChecked();
  auto res = V8Helper::createPropNameID(strRef, getIsolate());
  return res;
}

piper::PropNameID V8Runtime::createPropNameIDFromString(
    const piper::String& str) {
  return V8Helper::createPropNameID(V8Helper::stringRef(str), getIsolate());
}

std::string V8Runtime::utf8(const piper::PropNameID& sym) {
  return V8Helper::JSStringToSTLString(V8Helper::stringRef(sym), getContext());
}

bool V8Runtime::compare(const piper::PropNameID& a,
                        const piper::PropNameID& b) {
  return V8Helper::stringRef(a)
      ->Equals(getContext(), V8Helper::stringRef(b))
      .ToChecked();
}

std::optional<std::string> V8Runtime::symbolToString(const piper::Symbol& sym) {
  auto str = piper::Value(*this, sym).toString(*this);
  if (!str) {
    return std::optional<std::string>();
  }
  return str->utf8(*this);
}

piper::String V8Runtime::createStringFromAscii(const char* str, size_t length) {
  // Yes we end up double casting for semantic reasons (UTF8 contains ASCII,
  // not the other way around)
  return this->createStringFromUtf8(reinterpret_cast<const uint8_t*>(str),
                                    length);
}

piper::String V8Runtime::createStringFromUtf8(const uint8_t* str,
                                              size_t length) {
  std::string tmp(reinterpret_cast<const char*>(str), length);
  v8::Local<v8::String> stringRef =
      v8::String::NewFromUtf8(getIsolate(), reinterpret_cast<const char*>(str),
                              v8::NewStringType::kNormal,
                              static_cast<int>(length))
          .ToLocalChecked();
  return V8Helper::createString(stringRef, getIsolate());
}

std::string V8Runtime::utf8(const piper::String& str) {
  return V8Helper::JSStringToSTLString(V8Helper::stringRef(str), getContext());
}

piper::Object V8Runtime::createObject() {
  v8::Isolate* isolate_ = getIsolate();
  v8::Local<v8::Context> ctx = getContext();
  // Scope(*this);

  ENTER_SCOPE(ctx)
  return V8Helper::createObject(isolate_);
}

piper::Object V8Runtime::createObject(std::shared_ptr<piper::HostObject> ho) {
  Scope scope(*this);
  return V8HostObjectProxy::createObject(this, getContext(), std::move(ho));
}

std::weak_ptr<piper::HostObject> V8Runtime::getHostObject(
    const piper::Object& obj) {
  // We are guarenteed at this point to have isHostObject(obj) == true
  // so the private data should be HostObjectMetadata
  v8::Local<v8::Object> object = V8Helper::objectRef(obj);
  auto metadata = static_cast<detail::V8HostObjectProxy*>(
      object->GetAlignedPointerFromInternalField(0));
  DCHECK(metadata);
  return metadata->GetHost();
}

std::optional<Value> V8Runtime::getProperty(const piper::Object& obj,
                                            const piper::String& name) {
  v8::Isolate* isolate_ = getIsolate();
  v8::TryCatch trycatch(isolate_);

  v8::Local<v8::Object> objRef = V8Helper::objectRef(obj);
  v8::MaybeLocal<v8::Value> result =
      objRef->Get(getContext(), V8Helper::stringRef(name));

  if (result.IsEmpty()) {
    V8Exception::ReportExceptionIfNeeded(*this, trycatch);
    return std::optional<Value>();
  }
  return V8Helper::createValue(result.ToLocalChecked(), getContext());
}

std::optional<Value> V8Runtime::getProperty(const piper::Object& obj,
                                            const piper::PropNameID& name) {
  v8::Isolate* isolate_ = getIsolate();
  v8::TryCatch trycatch(isolate_);

  v8::Local<v8::Object> objRef = V8Helper::objectRef(obj);
  v8::MaybeLocal<v8::Value> result =
      objRef->Get(getContext(), V8Helper::stringRef(name));
  if (result.IsEmpty()) {
    V8Exception::ReportExceptionIfNeeded(*this, trycatch);
    return std::optional<Value>();
  }
  return V8Helper::createValue(result.ToLocalChecked(), getContext());
}

bool V8Runtime::hasProperty(const piper::Object& obj,
                            const piper::String& name) {
  Scope scope(*this);
  v8::Local<v8::Object> objRef = V8Helper::objectRef(obj);
  auto res = objRef->Has(getContext(), V8Helper::stringRef(name));
  return res.IsJust() && res.FromJust();
}

bool V8Runtime::hasProperty(const piper::Object& obj,
                            const piper::PropNameID& name) {
  Scope scope(*this);
  v8::Local<v8::Object> objRef = V8Helper::objectRef(obj);
  auto res = objRef->Has(getContext(), V8Helper::stringRef(name));
  return res.IsJust() && res.FromJust();
}

bool V8Runtime::setPropertyValue(piper::Object& object,
                                 const piper::PropNameID& name,
                                 const piper::Value& value) {
  v8::Isolate* isolate_ = getIsolate();

  Scope scope(*this);
  v8::TryCatch trycatch(isolate_);
  V8Helper::objectRef(object)
      ->Set(getContext(), V8Helper::stringRef(name), valueRef(value))
      .ToChecked();
  return V8Exception::ReportExceptionIfNeeded(*this, trycatch);
}

bool V8Runtime::setPropertyValue(piper::Object& object,
                                 const piper::String& name,
                                 const piper::Value& value) {
  v8::Isolate* isolate_ = getIsolate();

  Scope scope(*this);
  v8::TryCatch trycatch(isolate_);
  V8Helper::objectRef(object)
      ->Set(getContext(), V8Helper::stringRef(name), valueRef(value))
      .ToChecked();
  return V8Exception::ReportExceptionIfNeeded(*this, trycatch);
}

bool V8Runtime::isArray(const piper::Object& obj) const {
  auto v8_obj = V8Helper::objectRef(obj);
  if (v8_obj->IsArray()) {
    return true;
  }
  if (!v8_obj->IsProxy()) {
    return false;
  }
  auto proxy = v8_obj.As<v8::Proxy>();
  auto target = proxy->GetTarget();
  if (!target->IsArray()) {
    return false;
  }

  auto length_key = v8::String::NewFromUtf8(getIsolate(), "length",
                                            v8::NewStringType::kNormal)
                        .ToLocalChecked();
  auto length_value = v8_obj->Get(getContext(), length_key);

  return !length_value.IsEmpty() && length_value.ToLocalChecked()->IsUint32();
}

bool V8Runtime::isArrayBuffer(const piper::Object& obj) const {
  return V8Helper::objectRef(obj)->IsArrayBuffer();
}

uint8_t* V8Runtime::data(const piper::ArrayBuffer& obj) {
  Scope scope(*this);
  v8::Local<v8::ArrayBuffer> array_buffer;
  array_buffer = array_buffer.Cast(V8Helper::objectRef(obj));
#if V8_MAJOR_VERSION >= 9
  return static_cast<uint8_t*>(array_buffer->GetBackingStore()->Data());
#else
  return static_cast<uint8_t*>(array_buffer->GetContents().Data());
#endif
}

size_t V8Runtime::copyData(const ArrayBuffer& obj, uint8_t* dest_buf,
                           size_t dest_len) {
  Scope scope(*this);
  size_t src_len = obj.length(*this);
  if (dest_len < src_len) {
    return 0;
  }
  v8::Local<v8::ArrayBuffer> array_buffer;
  array_buffer = array_buffer.Cast(V8Helper::objectRef(obj));
#if V8_MAJOR_VERSION >= 9
  void* src_buf = array_buffer->GetBackingStore()->Data();
#else
  void* src_buf = array_buffer->GetContents().Data();
#endif

  memcpy(dest_buf, src_buf, src_len);
  return src_len;
}

size_t V8Runtime::size(const piper::ArrayBuffer& obj) {
  Scope scope(*this);
  v8::Local<v8::ArrayBuffer> array_buffer;
  array_buffer = array_buffer.Cast(V8Helper::objectRef(obj));
  return array_buffer->ByteLength();
}

bool V8Runtime::isFunction(const piper::Object& obj) const {
  return V8Helper::objectRef(obj)->IsFunction();
}

bool V8Runtime::isHostObject(const piper::Object& obj) const {
  v8::Local<v8::Object> object = V8Helper::objectRef(obj);
  if (object->InternalFieldCount() != V8HostObjectProxy::HOST_OBJ_COUNT) {
    return false;
  }
  void* ptr = object->GetAlignedPointerFromInternalField(0);
  return ptr != nullptr;
}

// Very expensive // TODO
std::optional<piper::Array> V8Runtime::getPropertyNames(
    const piper::Object& obj) {
  v8::Isolate* isolate_ = getIsolate();

  v8::TryCatch trycatch(isolate_);

  v8::Local<v8::Object> v8obj = V8Helper::objectRef(obj);
  v8::Local<v8::Array> ary =
      v8obj->GetPropertyNames(getContext()).ToLocalChecked();
  auto result = createArray(ary->Length());
  if (!result) {
    return std::optional<piper::Array>();
  }
  for (size_t i = 0; i < ary->Length(); i++) {
#if V8_MAJOR_VERSION >= 9
    v8::Local<v8::String> str = ary->Get(getContext(), i)
                                    .ToLocalChecked()
                                    ->ToString(getContext())
                                    .ToLocalChecked();
#else
    v8::Local<v8::String> str = ary->Get(i)->ToString(isolate_);
#endif

    if (!(*result).setValueAtIndex(*this, i,
                                   V8Helper::createString(str, isolate_))) {
      return std::optional<piper::Array>();
    }
  }
  return result;
}

// create BigInt object and and store value with key named "__lynx_val__", then
// add "toString" method to js object
std::optional<piper::BigInt> V8Runtime::createBigInt(const std::string& value,
                                                     Runtime& rt) {
  Scope scope(*this);
  v8::Isolate* isolate_ = getIsolate();

  v8::Local<v8::Object> obj = v8::Object::New(isolate_);

  // store value with key
  v8::Local<v8::String> key_str_v8 =
      V8Helper::ConvertToV8String(isolate_, "__lynx_val__");
  v8::Local<v8::String> val_str_v8 =
      V8Helper::ConvertToV8String(isolate_, value);

#if V8_MAJOR_VERSION >= 9
  (void)obj->Set(getContext(), key_str_v8, val_str_v8);
#else
  (void)obj->Set(key_str_v8, val_str_v8);
#endif

  // create "toString" function
  const std::string to_str = "toString";
  const lynx::piper::PropNameID prop =
      lynx::piper::PropNameID::forUtf8(rt, to_str);
  const lynx::piper::Value fun_value =
      lynx::piper::Function::createFromHostFunction(
          rt, prop, 0,
          [value](
              Runtime& rt, const Value& thisVal, const Value* args,
              unsigned int count) -> base::expected<Value, JSINativeException> {
            lynx::piper::String res =
                lynx::piper::String::createFromUtf8(rt, value);

            return piper::Value(rt, res);
          });
  // add "toString", "valueOf", "toJSON" property to js object as a function
  v8::Local<v8::String> to_str_v8 =
      V8Helper::ConvertToV8String(isolate_, to_str);
  const std::string value_of = "valueOf";
  v8::Local<v8::String> value_of_v8 =
      V8Helper::ConvertToV8String(isolate_, value_of);
  const std::string to_json = "toJSON";
  v8::Local<v8::String> to_json_v8 =
      V8Helper::ConvertToV8String(isolate_, to_json);

#if V8_MAJOR_VERSION >= 9
  (void)obj->Set(getContext(), to_str_v8, valueRef(fun_value));
  (void)obj->Set(getContext(), value_of_v8, valueRef(fun_value));
  (void)obj->Set(getContext(), to_json_v8, valueRef(fun_value));
#else
  (void)obj->Set(to_str_v8, valueRef(fun_value));
  (void)obj->Set(value_of_v8, valueRef(fun_value));
  (void)obj->Set(to_json_v8, valueRef(fun_value));
#endif

  return V8Helper::createObject(obj, getIsolate()).getBigInt(rt);
}

std::optional<Array> V8Runtime::createArray(size_t length) {
  Scope scope(*this);
  v8::Local<v8::Array> obj =
      v8::Array::New(getIsolate(), static_cast<int>(length));
  return V8Helper::createObject(obj, getIsolate()).getArray(*this);
}

piper::ArrayBuffer V8Runtime::createArrayBufferCopy(const uint8_t* bytes,
                                                    size_t byte_length) {
  Scope scope(*this);
  v8::TryCatch trycatch(getIsolate());
  v8::Local<v8::ArrayBuffer> obj;
  void* dst_buffer = nullptr;
  if (bytes && byte_length > 0) {
    dst_buffer = malloc(byte_length);
  }
  if (byte_length == 0) {
    obj = v8::ArrayBuffer::New(getIsolate(), 0);
  } else if (bytes && dst_buffer) {
    memcpy(dst_buffer, bytes, byte_length);
#if V8_MAJOR_VERSION >= 9
    auto store = v8::ArrayBuffer::NewBackingStore(
        dst_buffer, byte_length, [](void*, size_t, void*) {}, nullptr);
    obj = v8::ArrayBuffer::New(getIsolate(), std::move(store));
#else
    obj = v8::ArrayBuffer::New(getIsolate(), dst_buffer, byte_length,
                               v8::ArrayBufferCreationMode::kInternalized);
#endif
  }
  if (!V8Exception::ReportExceptionIfNeeded(*this, trycatch) || obj.IsEmpty()) {
    return piper::ArrayBuffer(*this);
  }
  return V8Helper::createObject(obj, getIsolate()).getArrayBuffer(*this);
}

piper::ArrayBuffer V8Runtime::createArrayBufferNoCopy(
    std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) {
  Scope scope(*this);
  v8::TryCatch trycatch(getIsolate());
  v8::Local<v8::ArrayBuffer> obj;
  if (byte_length == 0) {
    obj = v8::ArrayBuffer::New(getIsolate(), 0);
  } else if (bytes) {
    const uint8_t* raw_buffer = bytes.release();

#if V8_MAJOR_VERSION >= 9
    auto store = v8::ArrayBuffer::NewBackingStore(
        (void*)raw_buffer, byte_length, v8::BackingStore::EmptyDeleter,
        nullptr);

    obj = v8::ArrayBuffer::New(getIsolate(), std::move(store));
#else
    obj = v8::ArrayBuffer::New(getIsolate(), const_cast<uint8_t*>(raw_buffer),
                               byte_length,
                               v8::ArrayBufferCreationMode::kInternalized);
#endif
  }
  if (!V8Exception::ReportExceptionIfNeeded(*this, trycatch) || obj.IsEmpty()) {
    return piper::ArrayBuffer(*this);
  }
  return V8Helper::createObject(obj, getIsolate()).getArrayBuffer(*this);
}

std::optional<size_t> V8Runtime::size(const piper::Array& arr) {
  Scope scope(*this);
  auto v8_obj = V8Helper::objectRef(arr);
  if (v8_obj->IsArray()) {
    v8::Local<v8::Array> ary;
    ary = ary.Cast(v8_obj);
    return ary->Length();
  }
  if (!v8_obj->IsProxy()) {
    return std::nullopt;
  }

  auto length_key = v8::String::NewFromUtf8(getIsolate(), "length",
                                            v8::NewStringType::kNormal)
                        .ToLocalChecked();
  auto length_value = v8_obj->Get(getContext(), length_key);
  if (length_value.IsEmpty() || !length_value.ToLocalChecked()->IsUint32()) {
    return std::nullopt;
  }
  return length_value.ToLocalChecked()->Uint32Value(getContext()).ToChecked();
}

std::optional<Value> V8Runtime::getValueAtIndex(const piper::Array& arr,
                                                size_t i) {  // TODO
  v8::Local<v8::Object> obj = V8Helper::objectRef(arr);
#if V8_MAJOR_VERSION >= 9
  v8::Local<v8::Value> value = obj->Get(getContext(), i).ToLocalChecked();
#else
  v8::Local<v8::Value> value = obj->Get(i);
#endif
  return V8Helper::createValue(value, getContext());
}

bool V8Runtime::setValueAtIndexImpl(piper::Array& arr, size_t i,
                                    const piper::Value& value) {
  v8::Local<v8::Object> obj = V8Helper::objectRef(arr);
#if V8_MAJOR_VERSION >= 9
  (void)obj->Set(getContext(), i, valueRef(value));
#else
  (void)obj->Set(i, valueRef(value));
#endif
  return true;
}

bool V8Runtime::isHostFunction(const piper::Function& obj) const {
  v8::Local<v8::Object> object = V8Helper::objectRef(obj);
  auto res = object->HasPrivate(
      getContext(),
      v8::Private::New(getIsolate(),
                       V8Helper::ConvertToV8String(
                           getIsolate(), V8HostFunctionProxy::HOST_FUN_KEY)));
  return res.IsJust() && res.ToChecked();
}

// piper::HostFunctionType& V8Runtime::getHostFunction(
//  const piper::Function& obj) {
// We know that isHostFunction(obj) is true here, so its safe to proceed
//  auto proxy = static_cast<V8HostFunctionProxy*>(
//      JSObjectGetPrivate(V8Helper::objectRef(obj)));
// return proxy->getHostFunction();
// return nullptr;
//}
piper::Function V8Runtime::createFunctionFromHostFunction(
    const piper::PropNameID& name, unsigned int paramCount,
    piper::HostFunctionType func) {
  v8::Isolate* isolate_ = getIsolate();

  Scope scope(*this);
  v8::Local<v8::Object> v8_func =
      V8HostFunctionProxy::createFunctionFromHostFunction(
          this, getContext(), name, paramCount, std::move(func));
  return V8Helper::createObject(v8_func, isolate_).getFunction(*this);
}

std::optional<Value> V8Runtime::call(const piper::Function& f,
                                     const piper::Value& jsThis,
                                     const piper::Value* args, size_t count) {
  auto converter = ArgsConverter<v8::Local<v8::Value>>(
      count, args,
      [this](const piper::Value& value) { return valueRef(value); });
  return V8Helper::call(
      this, f, jsThis.isUndefined() ? Object(*this) : jsThis.getObject(*this),
      converter, count);
}

std::optional<Value> V8Runtime::callAsConstructor(const piper::Function& f,
                                                  const piper::Value* args,
                                                  size_t count) {
  auto converter = ArgsConverter<v8::Local<v8::Value>>(
      count, args,
      [this](const piper::Value& value) { return valueRef(value); });
  return V8Helper::callAsConstructor(this, V8Helper::objectRef(f), converter,
                                     static_cast<int>(count));
}

// TODO
Runtime::ScopeState* V8Runtime::pushScope() {
  v8::Isolate* isolate_ = getIsolate();

  isolate_->Enter();
  Runtime::ScopeState* state = new V8ScopeState(isolate_);
  v8::Local<v8::Context> ctx = getContext();
  ctx->Enter();
  return state;
}

void V8Runtime::popScope(ScopeState* state) {
  v8::Isolate* isolate_ = getIsolate();

  v8::Local<v8::Context> ctx = getContext();
  ctx->Exit();
  delete state;
  isolate_->Exit();
}

bool V8Runtime::strictEquals(const piper::Symbol& a,
                             const piper::Symbol& b) const {
  return V8Helper::symbolRef(a)->StrictEquals(V8Helper::symbolRef(b));
}

bool V8Runtime::strictEquals(const piper::String& a,
                             const piper::String& b) const {
  return V8Helper::stringRef(a)->StrictEquals(V8Helper::stringRef(b));
}

bool V8Runtime::strictEquals(const piper::Object& a,
                             const piper::Object& b) const {
  return V8Helper::objectRef(a)->StrictEquals(V8Helper::objectRef(b));
}

// TODO
bool V8Runtime::instanceOf(const piper::Object& o, const piper::Function& f) {
  return true;
}

v8::Local<v8::Value> V8Runtime::valueRef(const piper::Value& value) {
  switch (value.kind()) {
    case Value::ValueKind::UndefinedKind:
      return v8::Undefined(getIsolate());
    case Value::ValueKind::NullKind:
      return v8::Null(getIsolate());
    case Value::ValueKind::BooleanKind:
      return v8::Boolean::New(getIsolate(), value.getBool());
    case Value::ValueKind::NumberKind:
      return v8::Number::New(getIsolate(), value.getNumber());
    case Value::ValueKind::SymbolKind:
      return V8Helper::symbolRef(value.getSymbol(*this));
    case Value::ValueKind::StringKind:
      return V8Helper::stringRef(value.getString(*this));
    case Value::ValueKind::ObjectKind:
      return V8Helper::objectRef(value.getObject(*this));
  }
}

void V8Runtime::Finalize() { LOGI("V8Runtime::Finalize"); }

v8::Local<v8::ObjectTemplate> V8Runtime::GetHostObjectTemplate() {
  return host_object_template_.Get(getIsolate());
}

void V8Runtime::SetHostObjectTemplate(
    v8::Local<v8::ObjectTemplate> object_template) {
  host_object_template_.Reset(getIsolate(), object_template);
}

v8::Local<v8::ObjectTemplate> V8Runtime::GetHostFunctionTemplate() {
  return host_function_template_.Get(getIsolate());
}

void V8Runtime::SetHostFunctionTemplate(
    v8::Local<v8::ObjectTemplate> object_template) {
  host_function_template_.Reset(getIsolate(), object_template);
}

void V8Runtime::RequestGC() {
  LOGI("RequestGC");
  v8::Isolate* isolate = getIsolate();
  if (isolate) {
    isolate->LowMemoryNotification();
  }
}

void V8Runtime::InitInspector(
    const std::shared_ptr<InspectorRuntimeObserverNG>& observer) {
  if (observer != nullptr) {
    constexpr char kKeyEngineV8[] = "V8";
    auto inspector_manager =
        observer->CreateRuntimeInspectorManager(kKeyEngineV8);
    if (inspector_manager != nullptr) {
      inspector_manager_ = std::unique_ptr<V8InspectorManager>(
          static_cast<V8InspectorManager*>(inspector_manager.release()));
      inspector_manager_->InitInspector(this, observer);
    }
  }
}

void V8Runtime::DestroyInspector() {
  if (inspector_manager_ != nullptr) {
    inspector_manager_->DestroyInspector();
  }
}

std::unique_ptr<piper::Runtime> makeV8Runtime() {
  return std::make_unique<V8Runtime>();
}

std::shared_ptr<profile::V8RuntimeProfilerWrapper> makeV8RuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context) {
  auto vm = js_context->getVM();
  if (vm->GetRuntimeType() == piper::JSRuntimeType::v8) {
    std::shared_ptr<V8IsolateInstance> v8_vm =
        std::static_pointer_cast<V8IsolateInstance>(vm);
    auto v8_profiler = profile::V8RuntimeProfilerWrapperImpl::GetInstance();
    v8_profiler->Initialize(v8_vm);
    return std::shared_ptr<profile::V8RuntimeProfilerWrapper>(v8_profiler);
  }
  return nullptr;
}

}  // namespace piper
}  // namespace lynx
