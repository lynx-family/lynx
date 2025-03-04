// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/quickjs/quickjs_runtime.h"

#include <chrono>
#include <limits>
#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/runtime/common/args_converter.h"
#include "core/runtime/jscache/js_cache_manager.h"
#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider.h"
#include "core/runtime/jscache/quickjs/quickjs_cache_generator.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_api.h"
#include "core/runtime/jsi/quickjs/quickjs_exception.h"
#include "core/runtime/jsi/quickjs/quickjs_host_function.h"
#include "core/runtime/jsi/quickjs/quickjs_host_object.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/profile/quickjs/quickjs_runtime_profiler.h"
#include "core/services/event_report/event_tracker.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {

using detail::QuickjsHelper;
using detail::QuickjsHostFunctionProxy;
using detail::QuickjsHostObjectProxy;
using detail::QuickjsJSValueValue;

namespace {
void reportLepusToCStringError(Runtime &rt, const std::string &func_name,
                               int tag) {
  std::string error =
      func_name + " LepusToCString nullptr error! LepusValue's type tag is " +
      std::to_string(tag);
  LOGE(error);
  rt.reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(error));
}
}  // namespace

QuickjsRuntime::QuickjsRuntime() : quickjs_runtime_wrapper_(nullptr) {
#if !defined(LYNX_UNIT_TEST) || !LYNX_UNIT_TEST || \
    defined(QUICKJS_CACHE_UNITTEST)
  static std::once_flag clear_cache_flag;
  std::call_once(clear_cache_flag, []() {
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        []() {
          cache::JsCacheManager::GetQuickjsInstance().ClearInvalidCache();
        },
        base::ConcurrentTaskType::NORMAL_PRIORITY);
  });
#endif
};

lynx::piper::QuickjsRuntime::~QuickjsRuntime() {
  *is_runtime_destroyed_ = true;
  ClearHostContainers();
  context_->Release();
  context_.reset();
  LOGI("LYNX free quickjs context");
}

void QuickjsRuntime::InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                                 std::shared_ptr<JSIExceptionHandler> handler) {
  exception_handler_ = handler;
  quickjs_runtime_wrapper_ =
      std::static_pointer_cast<lynx::piper::QuickjsRuntimeInstance>(
          sharedContext->getVM());
  context_ = std::static_pointer_cast<QuickjsContextWrapper>(sharedContext);
  gc_flag_ = LEPUS_IsGCMode(getJSContext());
}

std::shared_ptr<VMInstance> QuickjsRuntime::getSharedVM() {
  return quickjs_runtime_wrapper_;
}

void QuickjsRuntime::SetGCPauseSuppressionMode(bool mode) {
  if (gc_flag_) LEPUS_SetGCPauseSuppressionMode(getJSRuntime(), mode);
}
bool QuickjsRuntime::GetGCPauseSuppressionMode() {
  if (gc_flag_) return LEPUS_GetGCPauseSuppressionMode(getJSRuntime());
  return false;
}

std::shared_ptr<VMInstance> QuickjsRuntime::createVM(
    const StartupData *) const {
  return CreateVM(nullptr, true);
}

std::shared_ptr<VMInstance> QuickjsRuntime::CreateVM(const StartupData *,
                                                     bool sync) {
  auto quickjs_runtime_wrapper =
      std::make_shared<lynx::piper::QuickjsRuntimeInstance>();
  quickjs_runtime_wrapper->InitQuickjsRuntime(sync);
  return quickjs_runtime_wrapper;
}

std::shared_ptr<JSIContext> QuickjsRuntime::createContext(
    std::shared_ptr<VMInstance> vm) const {
  return CreateContext_(vm);
}

std::shared_ptr<JSIContext> QuickjsRuntime::getSharedContext() {
  return context_;
}

std::shared_ptr<QuickjsContextWrapper> QuickjsRuntime::CreateContext_(
    std::shared_ptr<VMInstance> vm) const {
  return std::make_shared<QuickjsContextWrapper>(vm);
}

LEPUSClassID QuickjsRuntime::getFunctionClassID() const {
  return quickjs_runtime_wrapper_->getFunctionId();
};

LEPUSClassID QuickjsRuntime::getObjectClassID() const {
  return quickjs_runtime_wrapper_->getObjectId();
};

base::expected<Value, JSINativeException> QuickjsRuntime::evaluateJavaScript(
    const std::shared_ptr<const Buffer> &buffer,
    const std::string &source_url) {
  LOGI("QuickjsRuntime::evaluateJavaScript: " << source_url);
  std::string filename = BuildFilenameForDevTool(source_url);
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, "evaluateJavaScript", "source_url",
                      filename, "runtime_id", getRuntimeId());
  auto eval_res = QuickjsHelper::evalBuf(
      this, context_->getContext(),
      reinterpret_cast<const char *>(buffer->data()), buffer->size(),
      filename.c_str(), LEPUS_EVAL_TYPE_GLOBAL);
  if (!eval_res.has_value()) {
    LOGE("QuickjsRuntime::evaluateJavaScript failed:"
         << eval_res.error().ToString());
    return eval_res;
  }
  LOGI("QuickjsRuntime::evaluateJavaScript finished successfully.");
  return eval_res;
}

base::expected<Value, JSINativeException>
QuickjsRuntime::evaluateJavaScriptBytecode(
    const std::shared_ptr<const Buffer> &buffer,
    const std::string &source_url) {
  LOGI("QuickjsRuntime::evaluateJavaScriptBytecode: " << source_url);
  std::string filename = BuildFilenameForDevTool(source_url);
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, "evaluateJavaScriptBytecode",
                      "source_url", filename, "runtime_id", getRuntimeId());
  auto eval_res = QuickjsHelper::evalBin(
      this, context_->getContext(),
      reinterpret_cast<const char *>(buffer->data()), buffer->size(),
      filename.c_str(), LEPUS_EVAL_TYPE_GLOBAL);
  if (!eval_res.has_value()) {
    LOGE("QuickjsRuntime::evaluateJavaScriptBytecode failed:"
         << eval_res.error().ToString());
    return eval_res;
  }
  LOGI("QuickjsRuntime::evaluateJavaScriptBytecode finished successfully.");
  return eval_res;
}

std::shared_ptr<const PreparedJavaScript> QuickjsRuntime::prepareJavaScript(
    const std::shared_ptr<const Buffer> &buffer, std::string source_url) {
  std::shared_ptr<Buffer> cache;
#if !defined(LYNX_UNIT_TEST) || !LYNX_UNIT_TEST || \
    defined(QUICKJS_CACHE_UNITTEST)
  const auto cost_start = base::CurrentTimeMilliseconds();
  cache::JsCacheErrorCode error_code = cache::JsCacheErrorCode::NO_ERROR;
  if (IsJavaScriptBytecode(buffer)) {
    auto ret = PrepareJavaScriptBytecode(buffer, source_url, error_code);
    cache::JsCacheTracker::OnPrepareJS(
        JSRuntimeType::quickjs, source_url, ret != nullptr,
        cache::JsScriptType::BINARY,
        base::CurrentTimeMilliseconds() - cost_start, error_code);
    return ret;
  }
  // TODO(zhenziqi) consider compile js files in this function
  cache = GetBytecode(buffer, source_url);
  cache::JsCacheTracker::OnPrepareJS(
      JSRuntimeType::quickjs, source_url, false,
      cache ? cache::JsScriptType::LOCAL_BINARY : cache::JsScriptType::SOURCE,
      base::CurrentTimeMilliseconds() - cost_start, error_code);
#endif
  return std::make_shared<QuickjsJavaScriptPreparation>(buffer, cache,
                                                        std::move(source_url));
}

base::expected<Value, JSINativeException>
QuickjsRuntime::evaluatePreparedJavaScript(
    const std::shared_ptr<const PreparedJavaScript> &js) {
  if (!js) {
    LOGE(
        "QuickjsRuntime::evaluatePreparedJavaScript failed; PreparedJavaScript "
        "is null.");
    return base::unexpected<JSINativeException>(BUILD_JSI_NATIVE_EXCEPTION(
        "QuickjsRuntime::evaluatePreparedJavaScript failed; "
        "PreparedJavaScript is null."));
  }
  const QuickjsJavaScriptPreparation *preparation =
      static_cast<const QuickjsJavaScriptPreparation *>(js.get());
  LOGI("QuickjsRuntime::evaluatePreparedJavaScript start: "
       << preparation->SourceUrl());

  base::expected<Value, JSINativeException> eval_res;
  if (const auto bytecode = preparation->Bytecode()) {
    eval_res = evaluateJavaScriptBytecode(bytecode, preparation->SourceUrl());
    if (eval_res.has_value()) {
      return eval_res;
    }
#if !defined(LYNX_UNIT_TEST) || !LYNX_UNIT_TEST || \
    defined(QUICKJS_CACHE_UNITTEST)
    const auto source = preparation->Source();
    if (enable_user_bytecode_ && source) {
      cache::JsCacheManager::GetQuickjsInstance().RequestCacheGeneration(
          preparation->SourceUrl(), bytecode_source_url_, source,
          std::make_unique<cache::QuickjsCacheGenerator>(
              preparation->SourceUrl(), source),
          true);
    }
#endif
  }

  if (const auto source = preparation->Source()) {
    eval_res = evaluateJavaScript(source, preparation->SourceUrl());
  }
  if (!eval_res.has_value()) {
    LOGE("QuickjsRuntime::evaluatePreparedJavaScript failed:"
         << eval_res.error().ToString());
  }
  return eval_res;
}

Object lynx::piper::QuickjsRuntime::global() {
  LEPUSValue global_obj = LEPUS_GetGlobalObject(context_->getContext());
  auto ret = QuickjsHelper::createJSValue(context_->getContext(), global_obj);
  return ret;
}

LEPUSValue QuickjsRuntime::valueRef(const piper::Value &value) {
  switch (value.kind()) {
    case Value::ValueKind::UndefinedKind:
      return LEPUS_UNDEFINED;
    case Value::ValueKind::NullKind:
      return LEPUS_NULL;
    case Value::ValueKind::BooleanKind:
      return LEPUS_NewBool(context_->getContext(), value.getBool());
    case Value::ValueKind::NumberKind:
      return LEPUS_NewFloat64(context_->getContext(), value.getNumber());
    case Value::ValueKind::SymbolKind:
      return QuickjsHelper::symbolRef(value.getSymbol(*this));
    case Value::ValueKind::StringKind:
      return QuickjsHelper::stringRef(value.getString(*this));
    case Value::ValueKind::ObjectKind:
      return QuickjsHelper::objectRef(value.getObject(*this));
  }
}

lynx::piper::Runtime::PointerValue *lynx::piper::QuickjsRuntime::cloneSymbol(
    const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }
  const QuickjsJSValueValue *symbol =
      static_cast<const QuickjsJSValueValue *>(pv);
  return QuickjsHelper::makeJSValueValue(
      context_->getContext(),
      LEPUS_DupValue(context_->getContext(), symbol->Get()));
}

lynx::piper::Runtime::PointerValue *lynx::piper::QuickjsRuntime::cloneString(
    const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }
  const QuickjsJSValueValue *string =
      static_cast<const QuickjsJSValueValue *>(pv);
  return QuickjsHelper::makeStringValue(
      context_->getContext(),
      LEPUS_DupValue(context_->getContext(), string->Get()));
}

lynx::piper::Runtime::PointerValue *lynx::piper::QuickjsRuntime::cloneObject(
    const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }
  const QuickjsJSValueValue *object =
      static_cast<const QuickjsJSValueValue *>(pv);
  return QuickjsHelper::makeObjectValue(
      context_->getContext(),
      LEPUS_DupValue(context_->getContext(), object->Get()));
}

lynx::piper::Runtime::PointerValue *
lynx::piper::QuickjsRuntime::clonePropNameID(const Runtime::PointerValue *pv) {
  if (!pv) {
    return nullptr;
  }
  const QuickjsJSValueValue *string =
      static_cast<const QuickjsJSValueValue *>(pv);
  return QuickjsHelper::makeStringValue(
      context_->getContext(),
      LEPUS_DupValue(context_->getContext(), string->Get()));
}

lynx::piper::PropNameID lynx::piper::QuickjsRuntime::createPropNameIDFromAscii(
    const char *str, size_t length) {
  LEPUSValue value = LEPUS_NewStringLen(context_->getContext(), str, length);
  auto res = QuickjsHelper::createPropNameID(context_->getContext(), value);
  return res;
}

lynx::piper::PropNameID lynx::piper::QuickjsRuntime::createPropNameIDFromUtf8(
    const uint8_t *utf8, size_t length) {
  LEPUSValue value = LEPUS_NewStringLen(
      context_->getContext(), reinterpret_cast<const char *>(utf8), length);
  auto res = QuickjsHelper::createPropNameID(context_->getContext(), value);
  return res;
}

lynx::piper::PropNameID lynx::piper::QuickjsRuntime::createPropNameIDFromString(
    const lynx::piper::String &str) {
  return QuickjsHelper::createPropNameID(
      context_->getContext(),
      LEPUS_DupValue(context_->getContext(), QuickjsHelper::stringRef(str)));
}

std::string lynx::piper::QuickjsRuntime::utf8(
    const lynx::piper::PropNameID &sym) {
  return QuickjsHelper::LEPUSStringToSTLString(context_->getContext(),
                                               QuickjsHelper::valueRef(sym));
}

bool lynx::piper::QuickjsRuntime::compare(const lynx::piper::PropNameID &a,
                                          const lynx::piper::PropNameID &b) {
  std::string aa = QuickjsHelper::LEPUSStringToSTLString(
      context_->getContext(), QuickjsHelper::valueRef(a));
  std::string bb = QuickjsHelper::LEPUSStringToSTLString(
      context_->getContext(), QuickjsHelper::valueRef(b));
  return aa == bb;
}

std::optional<std::string> lynx::piper::QuickjsRuntime::symbolToString(
    const lynx::piper::Symbol &symbol) {
  auto str = piper::Value(*this, symbol).toString(*this);
  if (!str) {
    return std::optional<std::string>();
  }
  return str->utf8(*this);
}

lynx::piper::String lynx::piper::QuickjsRuntime::createStringFromAscii(
    const char *str, size_t length) {
  return this->createStringFromUtf8(reinterpret_cast<const uint8_t *>(str),
                                    length);
}

lynx::piper::String lynx::piper::QuickjsRuntime::createStringFromUtf8(
    const uint8_t *str, size_t length) {
  LEPUSValue value = LEPUS_NewStringLen(
      context_->getContext(), reinterpret_cast<const char *>(str), length);
  auto ret = QuickjsHelper::createString(context_->getContext(), value);
  return ret;
}

std::string lynx::piper::QuickjsRuntime::utf8(
    const lynx::piper::String &string) {
  return QuickjsHelper::LEPUSStringToSTLString(
      context_->getContext(), QuickjsHelper::stringRef(string));
}

lynx::piper::Object lynx::piper::QuickjsRuntime::createObject() {
  LEPUSValue value = LEPUS_NewObject(context_->getContext());
  //  LOGE( "LYNX" << "QuickjsRuntime::createObject() ptr=" <<
  //  LEPUS_VALUE_GET_PTR(value));
  return QuickjsHelper::createObject(context_->getContext(), value);
}

lynx::piper::Object lynx::piper::QuickjsRuntime::createObject(
    std::shared_ptr<HostObject> ho) {
  return QuickjsHostObjectProxy::createObject(this, std::move(ho));
}

std::weak_ptr<HostObject> lynx::piper::QuickjsRuntime::getHostObject(
    const piper::Object &object) {
  LEPUSValue obj = QuickjsHelper::objectRef(object);
  auto metadata = static_cast<detail::QuickjsHostObjectProxy *>(
      LEPUS_GetOpaque(obj, getObjectClassID()));
  DCHECK(metadata);
  return metadata->GetHost();
}

std::optional<Value> lynx::piper::QuickjsRuntime::getProperty(
    const lynx::piper::Object &object, const lynx::piper::PropNameID &name) {
  LEPUSValue v = QuickjsHelper::objectRef(object);
  LEPUSContext *ctx = context_->getContext();

  const char *prop = LEPUS_ToCString(ctx, QuickjsHelper::valueRef(name));
  if (!prop) {
    int64_t tag = LEPUS_VALUE_GET_TAG(QuickjsHelper::valueRef(name));
    reportLepusToCStringError(*this, "QuickjsRuntime::getProperty",
                              static_cast<int>(tag));
    return QuickjsHelper::createValue(LEPUS_UNDEFINED, this);
  }
  LEPUSValue result;
  if (gc_flag_) {
    HandleScope func_scope(ctx, (void *)&prop, HANDLE_TYPE_CSTRING);
    result = LEPUS_GetPropertyStr(context_->getContext(), v, prop);
  } else {
    result = LEPUS_GetPropertyStr(context_->getContext(), v, prop);
    LEPUS_FreeCString(context_->getContext(), prop);
  }
  QuickjsException::ReportExceptionIfNeeded(*this, result);
  auto ret = QuickjsHelper::createValue(result, this);
  return ret;
}

std::optional<Value> lynx::piper::QuickjsRuntime::getProperty(
    const lynx::piper::Object &object, const lynx::piper::String &name) {
  LEPUSValue v = QuickjsHelper::objectRef(object);
  LEPUSContext *ctx = context_->getContext();

  const char *prop = LEPUS_ToCString(ctx, QuickjsHelper::stringRef(name));
  if (!prop) {
    int64_t tag = LEPUS_VALUE_GET_TAG(QuickjsHelper::stringRef(name));
    reportLepusToCStringError(*this, "QuickjsRuntime::getProperty",
                              static_cast<int>(tag));
    return QuickjsHelper::createValue(LEPUS_UNDEFINED, this);
  }
  LEPUSValue result;
  if (gc_flag_) {
    HandleScope func_scope(ctx, (void *)&prop, HANDLE_TYPE_CSTRING);
    result = LEPUS_GetPropertyStr(context_->getContext(), v, prop);
  } else {
    result = LEPUS_GetPropertyStr(context_->getContext(), v, prop);
    LEPUS_FreeCString(context_->getContext(), prop);
  }
  QuickjsException::ReportExceptionIfNeeded(*this, result);
  auto ret = QuickjsHelper::createValue(result, this);
  return ret;
}

bool lynx::piper::QuickjsRuntime::hasProperty(
    const lynx::piper::Object &object, const lynx::piper::PropNameID &name) {
  LEPUSValue value = QuickjsHelper::objectRef(object);
  LEPUSContext *ctx = context_->getContext();
  const char *n = LEPUS_ToCString(ctx, QuickjsHelper::valueRef(name));
  if (!n) {
    int64_t tag = LEPUS_VALUE_GET_TAG(QuickjsHelper::valueRef(name));
    reportLepusToCStringError(*this, "QuickjsRuntime::hasProperty",
                              static_cast<int>(tag));
    return false;
  }
  HandleScope func_scope(ctx, &n, HANDLE_TYPE_CSTRING);
  LEPUSAtom atom = LEPUS_NewAtom(ctx, n);
  func_scope.PushLEPUSAtom(atom);
  auto ret = LEPUS_HasProperty(ctx, value, atom);

  if (!LEPUS_IsGCMode(ctx)) {
    LEPUS_FreeCString(ctx, n);
    LEPUS_FreeAtom(ctx, atom);
  }
  return ret;
}

bool lynx::piper::QuickjsRuntime::hasProperty(const lynx::piper::Object &object,
                                              const lynx::piper::String &name) {
  return hasProperty(object,
                     reinterpret_cast<const lynx::piper::PropNameID &>(name));
}

bool lynx::piper::QuickjsRuntime::setPropertyValue(
    lynx::piper::Object &object, const lynx::piper::PropNameID &name,
    const lynx::piper::Value &value) {
  LEPUSValue obj = QuickjsHelper::objectRef(object);
  LEPUSContext *ctx = context_->getContext();
  LEPUSValue property = LEPUS_DupValue(ctx, valueRef(value));
  const char *property_str =
      LEPUS_ToCString(ctx, QuickjsHelper::valueRef(name));
  if (!property_str) {
    int64_t tag = LEPUS_VALUE_GET_TAG(QuickjsHelper::valueRef(name));
    reportLepusToCStringError(*this, "QuickjsRuntime::setPropertyValue",
                              static_cast<int>(tag));
    return false;
  }
  HandleScope func_scope(ctx, (void *)&property_str, HANDLE_TYPE_CSTRING);
  int ret = LEPUS_SetPropertyStr(ctx, obj, property_str, property);

  if (!gc_flag_) LEPUS_FreeCString(ctx, property_str);
  if (ret == -1) {
    // TODO
    LOGE("setPropertyValue error" << name.utf8(*this));
  }
  return true;
}

bool lynx::piper::QuickjsRuntime::setPropertyValue(
    lynx::piper::Object &object, const lynx::piper::String &name,
    const lynx::piper::Value &value) {
  return setPropertyValue(
      object, reinterpret_cast<const lynx::piper::PropNameID &>(name), value);
}

bool lynx::piper::QuickjsRuntime::isArray(
    const lynx::piper::Object &object) const {
  return LEPUS_IsArray(context_->getContext(),
                       QuickjsHelper::objectRef(object));
}

bool lynx::piper::QuickjsRuntime::isArrayBuffer(
    const lynx::piper::Object &object) const {
  return LEPUS_IsArrayBuffer(QuickjsHelper::objectRef(object));
}

bool lynx::piper::QuickjsRuntime::isFunction(
    const lynx::piper::Object &object) const {
  return LEPUS_IsFunction(context_->getContext(),
                          QuickjsHelper::objectRef(object));
}

bool lynx::piper::QuickjsRuntime::isHostObject(
    const piper::Object &object) const {
  LEPUSValue value = QuickjsHelper::objectRef(object);
  return LEPUS_GetOpaque(value, getObjectClassID()) != nullptr;
}

bool lynx::piper::QuickjsRuntime::isHostFunction(
    const piper::Function &function) const {
  LEPUSValue value = QuickjsHelper::objectRef(function);
  return LEPUS_GetOpaque(value, getFunctionClassID()) != nullptr;
}

std::optional<piper::Array> lynx::piper::QuickjsRuntime::getPropertyNames(
    const lynx::piper::Object &object) {
  LEPUSValue obj = QuickjsHelper::objectRef(object);
  LEPUSPropertyEnum *tab_exotic;
  uint32_t exotic_count = 0;
  LEPUSAtom atom;
  uint32_t i;
  LEPUS_GetOwnPropertyNames(
      context_->getContext(), &tab_exotic, &exotic_count, obj,
      LEPUS_GPN_STRING_MASK | LEPUS_GPN_SYMBOL_MASK | LEPUS_GPN_ENUM_ONLY);
  HandleScope func_scope(context_->getContext(), tab_exotic,
                         HANDLE_TYPE_DIR_HEAP_OBJ);
  auto result = createArray(exotic_count);
  if (!result) {
    return std::optional<piper::Array>();
  }
  for (i = 0; i < exotic_count; i++) {
    atom = tab_exotic[i].atom;
    LEPUSValue name = LEPUS_AtomToValue(context_->getContext(), atom);
    if (!(*result).setValueAtIndex(
            *this, i,
            QuickjsHelper::createString(context_->getContext(), name))) {
      return std::optional<piper::Array>();
    }
  }
  uint32_t j;
  if (tab_exotic && !gc_flag_) {
    for (j = 0; j < exotic_count; j++)
      LEPUS_FreeAtom(context_->getContext(), tab_exotic[j].atom);
    lepus_free(context_->getContext(), tab_exotic);
  }
  return result;
}

std::optional<Array> lynx::piper::QuickjsRuntime::createArray(size_t length) {
  // https://tc39.es/ecma262/#sec-arraycreate
  if (length > std::numeric_limits<uint32_t>::max()) {
    // TODO(wangqingyu): Should throw a RangeError exception.
    return std::nullopt;
  }
  LEPUSValue arr = LEPUS_NewArray(context_->getContext());
  HandleScope block_scope(context_->getContext(), &arr,
                          HANDLE_TYPE_LEPUS_VALUE);
  LEPUS_SetPropertyStr(
      context_->getContext(), arr, "length",
      LEPUS_NewFloat64(context_->getContext(), static_cast<double>(length)));
  return QuickjsHelper::createObject(context_->getContext(), arr)
      .getArray(*this);
}

// create BigInt object and and store value with key named "__lynx_val__", then
// add "toString" method to js object
std::optional<piper::BigInt> lynx::piper::QuickjsRuntime::createBigInt(
    const std::string &value, Runtime &rt) {
  LEPUSValue obj = LEPUS_NewObject(context_->getContext());
  LEPUSContext *context = context_->getContext();
  auto piper_obj = QuickjsHelper::createObject(context, obj);

  // store value with key
  const std::string key = "__lynx_val__";
  lynx::piper::String value_str =
      lynx::piper::String::createFromUtf8(rt, value);
  // Must call the LEPUS_DupValue() to copy value before setting properties
  // otherwise it will crash
  LEPUS_DefinePropertyValueStr(context, obj, key.c_str(),
                               LEPUS_DupValue(context, valueRef(value_str)),
                               LEPUS_PROP_C_W_E);

  // create "toString" function
  const std::string to_str = "toString";
  const lynx::piper::PropNameID prop =
      lynx::piper::PropNameID::forUtf8(rt, to_str);
  const lynx::piper::Value fun_value =
      lynx::piper::Function::createFromHostFunction(
          rt, prop, 0,
          [value](Runtime &rt, const Value &thisVal, const Value *args,
                  size_t count) {
            lynx::piper::String res =
                lynx::piper::String::createFromUtf8(rt, value);

            return piper::Value(rt, res);
          });

  // add "toString" property to js object as a function
  LEPUS_DefinePropertyValueStr(context, obj, to_str.c_str(),
                               LEPUS_DupValue(context, valueRef(fun_value)),
                               LEPUS_PROP_C_W_E);

  // add "valueOf", "toJSON" property to js object as a function
  const std::string value_of = "valueOf";
  LEPUS_DefinePropertyValueStr(context, obj, value_of.c_str(),
                               LEPUS_DupValue(context, valueRef(fun_value)),
                               LEPUS_PROP_C_W_E);

  const std::string to_json = "toJSON";
  LEPUS_DefinePropertyValueStr(context, obj, to_json.c_str(),
                               LEPUS_DupValue(context, valueRef(fun_value)),
                               LEPUS_PROP_C_W_E);

  return std::move(piper_obj).getBigInt(rt);
}

lynx::piper::ArrayBuffer lynx::piper::QuickjsRuntime::createArrayBufferCopy(
    const uint8_t *bytes, size_t byte_length) {
  LEPUSValue array_buffer = LEPUS_UNDEFINED;
  if (byte_length == 0) {
    uint8_t bytes_array_buffer[1] = {0};
    array_buffer = LEPUS_NewArrayBufferCopy(context_->getContext(),
                                            bytes_array_buffer, byte_length);
  } else if (bytes) {
    array_buffer =
        LEPUS_NewArrayBufferCopy(context_->getContext(), bytes, byte_length);
  }
  if (!QuickjsException::ReportExceptionIfNeeded(*this, array_buffer) ||
      LEPUS_VALUE_GET_TAG(array_buffer) == LEPUS_TAG_UNDEFINED ||
      LEPUS_VALUE_GET_TAG(array_buffer) == LEPUS_TAG_NULL) {
    return lynx::piper::ArrayBuffer(*this);
  }
  return QuickjsHelper::createObject(context_->getContext(), array_buffer)
      .getArrayBuffer(*this);
}

lynx::piper::ArrayBuffer lynx::piper::QuickjsRuntime::createArrayBufferNoCopy(
    std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) {
  LEPUSFreeArrayBufferDataFunc *free_func = [](LEPUSRuntime *rt, void *opaque,
                                               void *ptr) {
    if (rt && ptr) {
      delete[] static_cast<uint8_t *>(ptr);
    }
  };
  LEPUSValue array_buffer = LEPUS_UNDEFINED;
  if (byte_length == 0) {
    array_buffer =
        LEPUS_NewArrayBuffer(context_->getContext(), new uint8_t[]{0},
                             byte_length, free_func, nullptr, false);
  } else if (bytes) {
    const uint8_t *raw_buffer = bytes.release();
    array_buffer = LEPUS_NewArrayBuffer(context_->getContext(),
                                        const_cast<uint8_t *>(raw_buffer),
                                        byte_length, free_func, nullptr, false);
  }
  if (!QuickjsException::ReportExceptionIfNeeded(*this, array_buffer) ||
      LEPUS_VALUE_GET_TAG(array_buffer) == LEPUS_TAG_UNDEFINED ||
      LEPUS_VALUE_GET_TAG(array_buffer) == LEPUS_TAG_NULL) {
    return lynx::piper::ArrayBuffer(*this);
  }
  return QuickjsHelper::createObject(context_->getContext(), array_buffer)
      .getArrayBuffer(*this);
}

std::optional<size_t> lynx::piper::QuickjsRuntime::size(
    const lynx::piper::Array &array) {
  LEPUSValue arr = QuickjsHelper::objectRef(array);
  LEPUSValue jsLength =
      LEPUS_GetPropertyStr(context_->getContext(), arr, "length");
  size_t l = LEPUS_VALUE_GET_INT(jsLength);
  return l;
}

size_t lynx::piper::QuickjsRuntime::size(
    const lynx::piper::ArrayBuffer &buffer) {
  size_t length = 0;
  LEPUS_GetArrayBuffer(context_->getContext(), &length,
                       QuickjsHelper::objectRef(buffer));
  return length;
}

uint8_t *lynx::piper::QuickjsRuntime::data(
    const lynx::piper::ArrayBuffer &array_buffer) {
  size_t length = 0;
  uint8_t *bytes = LEPUS_GetArrayBuffer(context_->getContext(), &length,
                                        QuickjsHelper::objectRef(array_buffer));
  return bytes;
}

size_t lynx::piper::QuickjsRuntime::copyData(const ArrayBuffer &array_buffer,
                                             uint8_t *dest_buf,
                                             size_t dest_len) {
  size_t src_len = array_buffer.length(*this);
  if (dest_len < src_len) {
    return 0;
  }
  size_t length = 0;
  uint8_t *bytes = LEPUS_GetArrayBuffer(context_->getContext(), &length,
                                        QuickjsHelper::objectRef(array_buffer));
  memcpy(dest_buf, bytes, length);
  return src_len;
}

std::optional<Value> lynx::piper::QuickjsRuntime::getValueAtIndex(
    const lynx::piper::Array &array, size_t i) {
  LEPUSValue arr = QuickjsHelper::objectRef(array);
  if (!LEPUS_IsArray(context_->getContext(), arr)) {
    LOGE("getValueAtIndex error. array is not an array");
    return piper::Value(nullptr);
  }
  LEPUSValue value = LEPUS_GetPropertyUint32(context_->getContext(), arr,
                                             static_cast<uint32_t>(i));
  //  LOGE( "LYNX getValueAtIndex jsvalueptr=" <<
  //  LEPUS_VALUE_GET_PTR(value));
  auto ret = QuickjsHelper::createValue(value, this);
  return ret;
}

bool lynx::piper::QuickjsRuntime::setValueAtIndexImpl(
    lynx::piper::Array &array, size_t i, const lynx::piper::Value &value) {
  LEPUSValue obj = QuickjsHelper::objectRef(array);
  //  LOGE( "LYNX setValueAtIndexImpl jsvalueptr=" <<
  //  LEPUS_VALUE_GET_PTR(obj));
  LEPUS_DefinePropertyValueUint32(
      context_->getContext(), obj, static_cast<uint32_t>(i),
      LEPUS_DupValue(context_->getContext(), valueRef(value)),
      LEPUS_PROP_C_W_E);
  return true;
}

lynx::piper::Function
lynx::piper::QuickjsRuntime::createFunctionFromHostFunction(
    const lynx::piper::PropNameID &name, unsigned int paramCount,
    HostFunctionType func) {
  LEPUSValue quick_func =
      QuickjsHostFunctionProxy::createFunctionFromHostFunction(
          this, context_->getContext(), name, paramCount, std::move(func));
  //  LOGE( "LYNX" << "createFunctionFromHostFunction ptr=" <<
  //  LEPUS_VALUE_GET_PTR(quick_func));
  return QuickjsHelper::createObject(context_->getContext(), quick_func)
      .getFunction(*this);
}

std::optional<Value> lynx::piper::QuickjsRuntime::call(
    const lynx::piper::Function &function, const lynx::piper::Value &jsThis,
    const lynx::piper::Value *args, size_t count) {
  auto converter = ArgsConverter<LEPUSValue>(
      count, args, [this](const auto &value) { return valueRef(value); });
  return QuickjsHelper::call(
      this, function,
      jsThis.isUndefined() ? QuickjsHelper::createObject(context_->getContext(),
                                                         LEPUS_UNINITIALIZED)
                           : jsThis.getObject(*this),
      converter, count);
}

std::optional<Value> lynx::piper::QuickjsRuntime::callAsConstructor(
    const lynx::piper::Function &function, const lynx::piper::Value *args,
    size_t count) {
  auto converter = ArgsConverter<LEPUSValue>(
      count, args, [this](const auto &value) { return valueRef(value); });
  return QuickjsHelper::callAsConstructor(this,
                                          QuickjsHelper::objectRef(function),
                                          converter, static_cast<int>(count));
}

lynx::piper::Runtime::ScopeState *lynx::piper::QuickjsRuntime::pushScope() {
  return Runtime::pushScope();
}

void lynx::piper::QuickjsRuntime::popScope(
    lynx::piper::Runtime::ScopeState *state) {
  Runtime::popScope(state);
}

bool lynx::piper::QuickjsRuntime::strictEquals(
    const lynx::piper::Symbol &a, const lynx::piper::Symbol &b) const {
  return LEPUS_VALUE_GET_PTR(QuickjsHelper::symbolRef(a)) ==
         LEPUS_VALUE_GET_PTR(QuickjsHelper::symbolRef(b));
}

bool lynx::piper::QuickjsRuntime::strictEquals(
    const lynx::piper::String &a, const lynx::piper::String &b) const {
  // LEPUS_StrictEq does the following for comparing two strings:
  //   1. Check if ptr are equals
  //     1.1 Return true if equals
  //   2. Check if they are atoms
  //     2.1 Return false if both are atoms
  //   3. Do the real string compare
  //   4. Free two strings
  // Thus, we should DupValue before calling LEPUS_StrictEq
  LEPUSContext *context = context_->getContext();
  return LEPUS_StrictEq(context,
                        LEPUS_DupValue(context, QuickjsHelper::stringRef(a)),
                        LEPUS_DupValue(context, QuickjsHelper::stringRef(b)));
}

bool lynx::piper::QuickjsRuntime::strictEquals(
    const lynx::piper::Object &a, const lynx::piper::Object &b) const {
  return LEPUS_VALUE_GET_PTR(QuickjsHelper::objectRef(a)) ==
         LEPUS_VALUE_GET_PTR(QuickjsHelper::objectRef(b));
}

bool lynx::piper::QuickjsRuntime::instanceOf(const lynx::piper::Object &o,
                                             const lynx::piper::Function &f) {
  int ret =
      LEPUS_IsInstanceOf(context_->getContext(), QuickjsHelper::objectRef(o),
                         QuickjsHelper::objectRef(f));
  return ret == 1;
}

void QuickjsRuntime::InitInspector(
    const std::shared_ptr<InspectorRuntimeObserverNG> &observer) {
  if (observer != nullptr) {
    constexpr char kKeyEngineQuickjs[] = "PrimJS";
    auto inspector_manager =
        observer->CreateRuntimeInspectorManager(kKeyEngineQuickjs);
    if (inspector_manager != nullptr) {
      inspector_manager_ = std::unique_ptr<QuickjsInspectorManager>(
          static_cast<QuickjsInspectorManager *>(inspector_manager.release()));
      inspector_manager_->InitInspector(this, observer);
    }
  }
}

void QuickjsRuntime::DestroyInspector() {
  if (inspector_manager_ != nullptr) {
    inspector_manager_->DestroyInspector();
  }
}

void QuickjsRuntime::RequestGC() {
  LOGI("RequestGC");
  if (auto rt = getJSRuntime()) {
    LEPUS_RunGC(rt);
  }
}

std::shared_ptr<Buffer> QuickjsRuntime::GetBytecode(
    const std::shared_ptr<const Buffer> &buffer,
    const std::string &source_url) const {
  std::shared_ptr<Buffer> cache;
#if !defined(LYNX_UNIT_TEST) || !LYNX_UNIT_TEST || \
    defined(QUICKJS_CACHE_UNITTEST)
  if (runtime::IsKernelJs(source_url) || enable_user_bytecode_) {
    LOGI("using new bytecode");
    auto &instance = cache::JsCacheManager::GetQuickjsInstance();
    auto generator =
        std::make_unique<cache::QuickjsCacheGenerator>(source_url, buffer);
    cache = instance.TryGetCache(source_url, bytecode_source_url_,
                                 getRuntimeId(), buffer, std::move(generator));
  } else if (!enable_user_bytecode_) {
    cache::JsCacheTracker::OnGetBytecodeDisable(
        getRuntimeId(), JSRuntimeType::quickjs, source_url, false, false);
  }
#endif
  return cache;
}

std::string QuickjsRuntime::BuildFilenameForDevTool(
    const std::string &source_url) {
  // sourceURL must have a prefix when support js debugging.
  if (inspector_manager_ != nullptr) {
    static constexpr char kUrlLynxCore[] = "lynx_core";

    std::string filename = inspector_manager_->BuildInspectorUrl(source_url);
    if (filename.find(kUrlLynxCore) == std::string::npos) {
      inspector_manager_->InsertScript(filename);
    }
    inspector_manager_->PrepareForScriptEval();
    return filename;
  }
  return source_url;
}

bool lynx::piper::QuickjsRuntime::setPropertyValueGC(
    Object &object, const char *name, const piper::Value &value) {
  LEPUSContext *ctx = context_->getContext();
  LEPUSValue obj = QuickjsHelper::objectRef(object);
  LEPUSValue property = valueRef(value);

  if (!name) {
    int tag = 0;  // LEPUS_VALUE_GET_TAG(QuickjsHelper::stringRef(name));
    reportLepusToCStringError(*this, "QuickjsRuntime::setPropertyValue", tag);
    return false;
  }
  int ret = LEPUS_SetPropertyStr(ctx, obj, name, property);
  if (ret == -1) {
    // TODO
    LOGE("setPropertyValue error" << name);
  }
  return true;
}

std::unique_ptr<piper::Runtime> makeQuickJsRuntime() {
  return std::make_unique<QuickjsRuntime>();
}

std::shared_ptr<VMInstance> CreateQuickJsVM(const StartupData *startup_data,
                                            bool sync) {
  return piper::QuickjsRuntime::CreateVM(startup_data, sync);
}

void BindQuickjsVMToCurrentThread(std::shared_ptr<piper::VMInstance> &vm) {
  if (!vm) {
    return;
  }
  piper::QuickjsRuntimeInstance *quickjs_vm =
      static_cast<piper::QuickjsRuntimeInstance *>(vm.get());
  quickjs_vm->AddToIdContainer();
}

BASE_EXPORT_FOR_DEVTOOL std::unique_ptr<profile::RuntimeProfiler>
makeQuickJsRuntimeProfiler(std::shared_ptr<piper::JSIContext> js_context) {
#if ENABLE_TRACE_PERFETTO
  auto vm = js_context->getVM();
  if (vm->GetRuntimeType() == piper::JSRuntimeType::quickjs) {
    auto quickjs_context =
        std::static_pointer_cast<piper::QuickjsContextWrapper>(js_context);
    return std::make_unique<profile::QuickjsRuntimeProfiler>(quickjs_context);
  }
#endif
  return nullptr;
}

bool QuickjsRuntime::IsJavaScriptBytecode(
    const std::shared_ptr<const Buffer> &buffer) {
  return quickjs::QuickjsBytecodeProvider::IsBytecode(buffer);
}

std::shared_ptr<const PreparedJavaScript>
QuickjsRuntime::PrepareJavaScriptBytecode(
    const std::shared_ptr<const Buffer> &buffer, std::string source_url,
    cache::JsCacheErrorCode &error_code) {
  auto provider = quickjs::QuickjsBytecodeProvider::FromPackedBytecode(buffer);
  if (!provider.has_value()) {
    reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "QuickjsRuntime::prepareJavaScript failed: " + provider.error()));
    error_code = cache::JsCacheErrorCode::BINARY_FORMAT_ERROR;
    return nullptr;
  }
  auto target_sdk_version = provider.value().GetTargetSdkVersion();
  if (target_sdk_version > LYNX_VERSION) {
    reportJSIException(
        BUILD_JSI_NATIVE_EXCEPTION("QuickjsRuntime::prepareJavaScript failed: "
                                   "invalid engine version: " +
                                   target_sdk_version.ToString()));
    error_code = cache::JsCacheErrorCode::TARGET_SDK_MISMATCH;
    return nullptr;
  }
  auto cache = provider.value().GetRawBytecode();
  return std::make_shared<QuickjsJavaScriptPreparation>(
      nullptr, std::move(cache), std::move(source_url));
}

}  // namespace piper
}  // namespace lynx
