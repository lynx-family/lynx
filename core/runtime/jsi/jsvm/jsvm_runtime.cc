// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/jsvm/jsvm_runtime.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "core/runtime/common/args_converter.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jslib.h"
#include "core/runtime/jsi/jsvm/jsvm_api.h"
#include "core/runtime/jsi/jsvm/jsvm_context_wrapper.h"
#include "core/runtime/jsi/jsvm/jsvm_dyn_load.h"
#include "core/runtime/jsi/jsvm/jsvm_helper.h"
#include "core/runtime/jsi/jsvm/jsvm_host_function.h"
#include "core/runtime/jsi/jsvm/jsvm_host_object.h"
#include "core/runtime/jsi/jsvm/jsvm_runtime_wrapper.h"
#include "core/runtime/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace piper {
using detail::JSVMHelper;

std::shared_ptr<piper::Runtime> makeJSVMRuntime() {
  return std::make_shared<JSVMRuntime>();
}

std::shared_ptr<profile::RuntimeProfiler> makeJSVMRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context) {
  // TODO(yangguangzhao.solace): implement me
  return nullptr;
}

bool IsJSVMRuntimeAvailable() {
  return DynamicLoader::GetInstance()->IsJsvmAvailable();
}

JSVMRuntime::~JSVMRuntime() {
  ClearHostContainers();
  JSVM_CALL(OH_JSVM_DeleteReference, (getEnv(), host_object_template_));
  host_object_template_ = nullptr;
  JSVM_CALL(OH_JSVM_DeleteReference, (getEnv(), host_function_template_));
  host_function_template_ = nullptr;
}

void JSVMRuntime::InitRuntime(std::shared_ptr<JSIContext> sharedContext,
                              std::shared_ptr<JSIExceptionHandler> handler) {
  exception_handler_ = handler;
  runtime_wrapper_ =
      std::static_pointer_cast<JSVMRuntimeInstance>(sharedContext->getVM());
  context_ = std::static_pointer_cast<JSVMContextWrapper>(sharedContext);
}

std::shared_ptr<VMInstance> JSVMRuntime::createVM(
    const StartupData* data) const {
  auto instance_wrapper = std::make_shared<JSVMRuntimeInstance>();
  instance_wrapper->InitInstance();
  return instance_wrapper;
}

std::shared_ptr<VMInstance> JSVMRuntime::getSharedVM() {
  return runtime_wrapper_;
}

std::shared_ptr<JSIContext> JSVMRuntime::createContext(
    std::shared_ptr<VMInstance> vm) const {
  auto context_wrapper = std::make_shared<JSVMContextWrapper>(vm);
  return context_wrapper;
}

std::shared_ptr<JSIContext> JSVMRuntime::getSharedContext() { return context_; }

std::shared_ptr<const PreparedJavaScript> JSVMRuntime::prepareJavaScript(
    const std::shared_ptr<const Buffer>& buffer, std::string sourceURL) {
  return std::make_shared<piper::SourceJavaScriptPreparation>(
      buffer, std::move(sourceURL));
}

base::expected<Value, JSINativeException>
JSVMRuntime::evaluatePreparedJavaScript(
    const std::shared_ptr<const PreparedJavaScript>& js) {
  const SourceJavaScriptPreparation* source =
      static_cast<const SourceJavaScriptPreparation*>(js.get());
  return evaluateJavaScript(source->buffer(), source->sourceURL());
}

base::expected<Value, JSINativeException> JSVMRuntime::evaluateJavaScript(
    const std::shared_ptr<const Buffer>& buffer, const std::string& sourceURL) {
  LOGI("JSVMRuntime::evaluateJavaScript start url=" << sourceURL);
  JSVM_Env env = getEnv();
  HandleScopeWrapper scope(env);

  JSVM_Value js_source = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_CreateStringUtf8,
                   (env, reinterpret_cast<const char*>(buffer->data()),
                    buffer->size(), &js_source),
                   Value::undefined());

  bool cacheRejected = true;
  JSVM_Script script = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_CompileScript,
                   (env, js_source, nullptr, 0, true, &cacheRejected, &script),
                   Value::undefined());

  JSVM_Value result = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_RunScript, (env, script, &result),
                   Value::undefined());

  auto ret = JSVMHelper::createValue(result, env);
  return ret;
}

base::expected<Value, JSINativeException>
JSVMRuntime::evaluateJavaScriptBytecode(
    const std::shared_ptr<const Buffer>& buffer,
    const std::string& source_url) {
  LOGE("evaluateJavaScriptBytecode not supported in harmony jsvm");
  return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
      "evaluateJavaScriptBytecode not supported in harmony jsvm"));
}

Object JSVMRuntime::global() {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value global = nullptr;
  JSVM_CALL(OH_JSVM_GetGlobal, (getEnv(), &global));

  return JSVMHelper::createObject(global, getEnv());
}

void JSVMRuntime::valueRef(const piper::Value& value, JSVM_Value* result) {
  JSVM_Env env = getEnv();

  switch (value.kind()) {
    case Value::ValueKind::UndefinedKind:
      JSVM_CALL(OH_JSVM_GetUndefined, (env, result));
      break;
    case Value::ValueKind::NullKind:
      JSVM_CALL(OH_JSVM_GetNull, (env, result));
      break;
    case Value::ValueKind::BooleanKind:
      JSVM_CALL(OH_JSVM_GetBoolean, (env, value.getBool(), result));
      break;
    case Value::ValueKind::NumberKind:
      JSVM_CALL(OH_JSVM_CreateDouble, (env, value.getNumber(), result));
      break;
    case Value::ValueKind::SymbolKind:
      JSVMHelper::symbolRef(value.getSymbol(*this), result);
      break;
    case Value::ValueKind::StringKind:
      JSVMHelper::stringRef(value.getString(*this), result);
      break;
    case Value::ValueKind::ObjectKind:
      JSVMHelper::objectRef(value.getObject(*this), result);
      break;
    default:
      abort();
  }
}

////////////////////////
// protected
////////////////////////

Runtime::PointerValue* JSVMRuntime::cloneSymbol(
    const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }

  const detail::JSVMSymbolValue* symbol =
      static_cast<const detail::JSVMSymbolValue*>(pv);
  HandleScopeWrapper scope(symbol->env_);
  JSVM_Value sym_val = nullptr;
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (symbol->env_, symbol->sym_ref_, &sym_val));
  return JSVMHelper::makeSymbolValue(sym_val, symbol->env_);
}

Runtime::PointerValue* JSVMRuntime::cloneString(
    const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }

  const detail::JSVMStringValue* string =
      static_cast<const detail::JSVMStringValue*>(pv);
  HandleScopeWrapper scope(string->env_);
  JSVM_Value str_val = nullptr;
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (string->env_, string->str_ref_, &str_val));
  return JSVMHelper::makeStringValue(str_val, string->env_);
}

Runtime::PointerValue* JSVMRuntime::cloneObject(
    const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }

  const detail::JSVMObjectValue* object =
      static_cast<const detail::JSVMObjectValue*>(pv);
  HandleScopeWrapper scope(object->env_);
  JSVM_Value obj_val = nullptr;
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (object->env_, object->obj_ref_, &obj_val));
  return JSVMHelper::makeObjectValue(obj_val, object->env_);
}

Runtime::PointerValue* JSVMRuntime::clonePropNameID(
    const Runtime::PointerValue* pv) {
  if (!pv) {
    return nullptr;
  }

  const detail::JSVMStringValue* string =
      static_cast<const detail::JSVMStringValue*>(pv);
  HandleScopeWrapper scope(string->env_);
  JSVM_Value str_val = nullptr;
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (string->env_, string->str_ref_, &str_val));
  return JSVMHelper::makeStringValue(str_val, getEnv());
}

piper::PropNameID JSVMRuntime::createPropNameIDFromAscii(const char* str,
                                                         size_t length) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value valueStr = nullptr;
  JSVM_CALL(OH_JSVM_CreateStringUtf8, (getEnv(), str, length, &valueStr));
  auto res = JSVMHelper::createPropNameID(valueStr, getEnv());
  return res;
}

piper::PropNameID JSVMRuntime::createPropNameIDFromUtf8(const uint8_t* utf8,
                                                        size_t length) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value valueStr = nullptr;
  JSVM_CALL(OH_JSVM_CreateStringUtf8,
            (getEnv(), reinterpret_cast<const char*>(utf8), length, &valueStr));
  auto res = JSVMHelper::createPropNameID(valueStr, getEnv());
  return res;
}

piper::PropNameID JSVMRuntime::createPropNameIDFromString(
    const piper::String& str) {
  JSVM_Value str_value = nullptr;
  JSVMHelper::stringRef(str, &str_value);
  return JSVMHelper::createPropNameID(str_value, getEnv());
}

std::string JSVMRuntime::utf8(const piper::PropNameID& sym) {
  HandleScopeWrapper scope(getEnv());
  auto jsvm_str = static_cast<const detail::JSVMStringValue*>(
      Runtime::getPointerValue(sym));

  size_t size;
  JSVM_Value str_value = nullptr;
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (getEnv(), jsvm_str->str_ref_, &str_value));
  JSVM_CALL(OH_JSVM_GetValueStringUtf8,
            (getEnv(), str_value, nullptr, JSVM_AUTO_LENGTH, &size));

  std::string output_str;
  output_str.resize(size + 1);
  JSVM_CALL(OH_JSVM_GetValueStringUtf8, (getEnv(), str_value, output_str.data(),
                                         output_str.size(), nullptr));
  return output_str.substr(0, size);
}

bool JSVMRuntime::compare(const piper::PropNameID& a,
                          const piper::PropNameID& b) {
  JSVM_Value a_value = nullptr;
  JSVMHelper::stringRef(a, &a_value);
  JSVM_Value b_value = nullptr;
  JSVMHelper::stringRef(b, &b_value);

  bool result = false;
  JSVM_CALL(OH_JSVM_StrictEquals, (getEnv(), a_value, b_value, &result));
  return result;
}

std::optional<std::string> JSVMRuntime::symbolToString(
    const piper::Symbol& sym) {
  auto str = piper::Value(*this, sym).toString(*this);
  if (!str) {
    return std::optional<std::string>();
  }
  return str->utf8(*this);
}

piper::String JSVMRuntime::createStringFromAscii(const char* str,
                                                 size_t length) {
  // Yes we end up double casting for semantic reasons (UTF8 contains ASCII,
  // not the other way around)
  return this->createStringFromUtf8(reinterpret_cast<const uint8_t*>(str),
                                    length);
}

piper::String JSVMRuntime::createStringFromUtf8(const uint8_t* str,
                                                size_t length) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value str_value = nullptr;
  JSVM_CALL(OH_JSVM_CreateStringUtf8,
            (getEnv(), reinterpret_cast<const char*>(str), length, &str_value));
  return JSVMHelper::createString(str_value, getEnv());
}

std::string JSVMRuntime::utf8(const piper::String& str) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value str_value = nullptr;
  JSVMHelper::stringRef(str, &str_value);
  return JSVMHelper::JSStringToSTLString(str_value, getEnv());
}

piper::Object JSVMRuntime::createObject() {
  HandleScopeWrapper scope(getEnv());
  return JSVMHelper::createObject(getEnv());
}

piper::Object JSVMRuntime::createObject(std::shared_ptr<piper::HostObject> ho) {
  return detail::JSVMHostObjectProxy::createObject(this, getEnv(), ho);
}

std::weak_ptr<piper::HostObject> JSVMRuntime::getHostObject(
    const piper::Object& obj) {
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);
  detail::JSVMHostObjectProxy* proxy_ptr = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_Unwrap,
                   (getEnv(), obj_value, reinterpret_cast<void**>(&proxy_ptr)),
                   std::weak_ptr<piper::HostObject>());
  return proxy_ptr->GetHost();
}

std::optional<Value> JSVMRuntime::getProperty(const piper::Object& obj,
                                              const piper::String& name) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value prop_value = nullptr;
  JSVMHelper::objectRef(obj, &prop_value);

  JSVM_Value target_value = nullptr;
  JSVM_CALL(OH_JSVM_GetNamedProperty,
            (getEnv(), prop_value, name.utf8(*this).c_str(), &target_value));
  return JSVMHelper::createValue(target_value, getEnv());
}

std::optional<Value> JSVMRuntime::getProperty(const piper::Object& obj,
                                              const piper::PropNameID& name) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value prop_value = nullptr;
  JSVMHelper::objectRef(obj, &prop_value);

  JSVM_Value name_value = nullptr;
  JSVMHelper::stringRef(name, &name_value);

  JSVM_Value target_value = nullptr;
  JSVM_CALL(OH_JSVM_GetProperty,
            (getEnv(), prop_value, name_value, &target_value));
  return JSVMHelper::createValue(target_value, getEnv());
}

bool JSVMRuntime::hasProperty(const piper::Object& obj,
                              const piper::String& name) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);
  bool result;
  JSVM_CALL(OH_JSVM_HasNamedProperty,
            (getEnv(), obj_value, name.utf8(*this).c_str(), &result));
  return result;
}

bool JSVMRuntime::hasProperty(const piper::Object& obj,
                              const piper::PropNameID& name) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);

  JSVM_Value name_value = nullptr;
  JSVMHelper::stringRef(name, &name_value);

  bool result;
  JSVM_CALL(OH_JSVM_HasProperty, (getEnv(), obj_value, name_value, &result));
  return result;
}

bool JSVMRuntime::setPropertyValue(piper::Object& object,
                                   const piper::PropNameID& name,
                                   const piper::Value& value) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_jsvm = nullptr;
  JSVMHelper::objectRef(object, &obj_jsvm);

  JSVM_Value name_jsvm = nullptr;
  JSVMHelper::stringRef(name, &name_jsvm);

  JSVM_Value value_jsvm = nullptr;
  valueRef(value, &value_jsvm);
  JSVM_CALL(OH_JSVM_SetProperty, (getEnv(), obj_jsvm, name_jsvm, value_jsvm));
  return true;
}

bool JSVMRuntime::setPropertyValue(piper::Object& object,
                                   const piper::String& name,
                                   const piper::Value& value) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(object, &obj_value);

  JSVM_Value name_value = nullptr;
  JSVMHelper::stringRef(name, &name_value);

  JSVM_Value value_value = nullptr;
  valueRef(value, &value_value);
  JSVM_CALL_RETURN(OH_JSVM_SetNamedProperty,
                   (getEnv(), obj_value, name.utf8(*this).c_str(), value_value),
                   false);
  return true;
}

bool JSVMRuntime::isArray(const piper::Object& obj) const {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);

  bool result;
  JSVM_CALL_RETURN(OH_JSVM_IsArray, (getEnv(), obj_value, &result), false);
  return result;
}

bool JSVMRuntime::isArrayBuffer(const piper::Object& obj) const {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);

  bool result;
  JSVM_CALL_RETURN(OH_JSVM_IsArraybuffer, (getEnv(), obj_value, &result),
                   false);
  return result;
}

bool JSVMRuntime::isFunction(const piper::Object& obj) const {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value func_value = nullptr;
  JSVMHelper::objectRef(obj, &func_value);

  bool result;
  JSVM_CALL_RETURN(OH_JSVM_IsFunction, (getEnv(), func_value, &result), false);
  return result;
}

bool JSVMRuntime::isHostObject(const piper::Object& obj) const {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);
  bool result;
  JSVM_CALL_RETURN(OH_JSVM_CheckObjectTypeTag,
                   (getEnv(), obj_value,
                    detail::JSVMHostObjectProxy::GetHostObjectTag(), &result),
                   false);
  return result;
}

bool JSVMRuntime::isHostFunction(const piper::Function& obj) const {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);
  bool result;
  JSVM_CALL_RETURN(
      OH_JSVM_CheckObjectTypeTag,
      (getEnv(), obj_value, detail::JSVMHostFunctionProxy::GetHostFunctionTag(),
       &result),
      false);
  return result;
}

std::optional<piper::Array> JSVMRuntime::getPropertyNames(
    const piper::Object& obj) {
  HandleScopeWrapper scope(getEnv());

  JSVM_Value instance_value = nullptr;
  JSVMHelper::objectRef(obj, &instance_value);

  JSVM_Value all_prop_names = nullptr;
  JSVM_CALL(
      OH_JSVM_GetAllPropertyNames,
      (getEnv(), instance_value, JSVM_KEY_OWN_ONLY,
       static_cast<JSVM_KeyFilter>(JSVM_KEY_ENUMERABLE | JSVM_KEY_SKIP_SYMBOLS),
       JSVM_KEY_NUMBERS_TO_STRINGS, &all_prop_names));
  uint32_t name_size = 0;
  JSVM_CALL(OH_JSVM_GetArrayLength, (getEnv(), all_prop_names, &name_size));

  auto result = createArray(name_size);
  if (!result) {
    return std::optional<piper::Array>();
  }

  JSVM_Value prop_name = nullptr;
  for (uint32_t i = 0; i < name_size; ++i) {
    JSVM_CALL(OH_JSVM_GetElement, (getEnv(), all_prop_names, i, &prop_name));

    if (!(*result).setValueAtIndex(
            *this, i, JSVMHelper::createString(prop_name, getEnv()))) {
      return std::optional<piper::Array>();
    }
  }
  return result;
}

std::optional<BigInt> JSVMRuntime::createBigInt(const std::string& value,
                                                Runtime& rt) {
  HandleScopeWrapper scope(getEnv());

  JSVM_Value obj_value = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_CreateObject, (getEnv(), &obj_value),
                   std::optional<BigInt>());

  JSVM_Value key_str_jsvm = nullptr;
  JSVMHelper::ConvertToJSVMString(getEnv(), "__lynx_val__", &key_str_jsvm);
  JSVM_Value val_str_jsvm = nullptr;
  JSVMHelper::ConvertToJSVMString(getEnv(), value, &val_str_jsvm);
  JSVM_CALL_RETURN(OH_JSVM_SetProperty,
                   (getEnv(), obj_value, key_str_jsvm, val_str_jsvm),
                   std::optional<BigInt>());

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
  JSVM_Value fun_value_jsvm = nullptr;
  valueRef(fun_value, &fun_value_jsvm);

  // add "toString", "valueOf", "toJSON" property to js object as a function
  JSVM_Value to_str_jsvm = nullptr;
  JSVMHelper::ConvertToJSVMString(getEnv(), to_str, &to_str_jsvm);
  const std::string value_of = "valueOf";
  JSVM_Value value_of_jsvm = nullptr;
  JSVMHelper::ConvertToJSVMString(getEnv(), value_of, &value_of_jsvm);
  const std::string to_json = "toJSON";
  JSVM_Value to_json_jsvm = nullptr;
  JSVMHelper::ConvertToJSVMString(getEnv(), to_json, &to_json_jsvm);

  JSVM_CALL_RETURN(OH_JSVM_SetProperty,
                   (getEnv(), obj_value, to_str_jsvm, fun_value_jsvm),
                   std::optional<BigInt>());
  JSVM_CALL_RETURN(OH_JSVM_SetProperty,
                   (getEnv(), obj_value, value_of_jsvm, fun_value_jsvm),
                   std::optional<BigInt>());
  JSVM_CALL_RETURN(OH_JSVM_SetProperty,
                   (getEnv(), obj_value, to_json_jsvm, fun_value_jsvm),
                   std::optional<BigInt>());

  return JSVMHelper::createObject(obj_value, getEnv()).getBigInt(rt);
}

std::optional<Array> JSVMRuntime::createArray(size_t length) {
  HandleScopeWrapper scope(getEnv());

  JSVM_Value arr_value = nullptr;
  JSVM_CALL(OH_JSVM_CreateArrayWithLength, (getEnv(), length, &arr_value));

  return JSVMHelper::createObject(arr_value, getEnv()).getArray(*this);
}

piper::ArrayBuffer JSVMRuntime::createArrayBufferCopy(const uint8_t* bytes,
                                                      size_t byte_length) {
  void* dst_buffer = nullptr;
  JSVM_Value result = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_CreateArraybuffer,
                   (getEnv(), byte_length, &dst_buffer, &result),
                   piper::ArrayBuffer(*this));
  if (byte_length > 0) {
    memcpy(dst_buffer, bytes, byte_length);
  }

  return JSVMHelper::createObject(result, getEnv()).getArrayBuffer(*this);
}

piper::ArrayBuffer JSVMRuntime::createArrayBufferNoCopy(
    std::unique_ptr<const uint8_t[]> bytes, size_t byte_length) {
  JSVM_Value result = nullptr;
  uint8_t* raw_buffer = const_cast<uint8_t*>(bytes.release());
  JSVM_CALL_RETURN(OH_JSVM_CreateArrayBufferFromBackingStoreData,
                   (getEnv(), reinterpret_cast<void*>(raw_buffer), byte_length,
                    0, byte_length, &result),
                   piper::ArrayBuffer(*this));

  return JSVMHelper::createObject(result, getEnv()).getArrayBuffer(*this);
}

std::optional<size_t> JSVMRuntime::size(const piper::Array& arr) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj = nullptr;
  JSVMHelper::objectRef(arr, &obj);
  uint32_t result;
  JSVM_CALL_RETURN(OH_JSVM_GetArrayLength, (getEnv(), obj, &result),
                   std::nullopt);

  return result;
}

size_t JSVMRuntime::size(const piper::ArrayBuffer& obj) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);
  size_t result;
  JSVM_CALL_RETURN(OH_JSVM_GetArraybufferInfo,
                   (getEnv(), obj_value, nullptr, &result), 0);
  return result;
}

uint8_t* JSVMRuntime::data(const piper::ArrayBuffer& obj) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);

  size_t size;
  void* data;
  JSVM_CALL_RETURN(OH_JSVM_GetArraybufferInfo,
                   (getEnv(), obj_value, &data, &size), nullptr);
  return reinterpret_cast<uint8_t*>(data);
}

size_t JSVMRuntime::copyData(const ArrayBuffer& obj, uint8_t* dest_buf,
                             size_t dest_len) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(obj, &obj_value);

  size_t src_len;
  void* src_buf;
  JSVM_CALL_RETURN(OH_JSVM_GetArraybufferInfo,
                   (getEnv(), obj_value, &src_buf, &src_len), 0);

  memcpy(dest_buf, src_buf, src_len);
  return src_len;
}

std::optional<Value> JSVMRuntime::getValueAtIndex(const piper::Array& arr,
                                                  size_t i) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj = nullptr;
  JSVMHelper::objectRef(arr, &obj);
  JSVM_Value result = nullptr;
  JSVM_CALL(OH_JSVM_GetElement, (getEnv(), obj, i, &result));

  return JSVMHelper::createValue(result, getEnv());
}

bool JSVMRuntime::setValueAtIndexImpl(piper::Array& arr, size_t i,
                                      const piper::Value& value) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value obj = nullptr;
  JSVMHelper::objectRef(arr, &obj);
  JSVM_Value result = nullptr;
  valueRef(value, &result);
  JSVM_CALL(OH_JSVM_SetElement, (getEnv(), obj, i, result));
  return true;
}

piper::Function JSVMRuntime::createFunctionFromHostFunction(
    const piper::PropNameID& name, unsigned int paramCount,
    piper::HostFunctionType func) {
  HandleScopeWrapper scope(getEnv());
  JSVM_Value func_value = nullptr;
  func_value = detail::JSVMHostFunctionProxy::createFunctionFromHostFunction(
      this, getEnv(), name, paramCount, std::move(func));
  return JSVMHelper::createObject(func_value, getEnv()).getFunction(*this);
}

std::optional<Value> JSVMRuntime::call(const piper::Function& f,
                                       const piper::Value& jsThis,
                                       const piper::Value* args, size_t count) {
  HandleScopeWrapper scope(getEnv());
  auto converter =
      ArgsConverter<JSVM_Value>(count, args, [this](const piper::Value& value) {
        JSVM_Value result = nullptr;
        valueRef(value, &result);
        return result;
      });
  return JSVMHelper::call(
      this, f, jsThis.isUndefined() ? Object(*this) : jsThis.getObject(*this),
      converter, count);
}

std::optional<Value> JSVMRuntime::callAsConstructor(const piper::Function& f,
                                                    const piper::Value* args,
                                                    size_t count) {
  auto converter =
      ArgsConverter<JSVM_Value>(count, args, [this](const piper::Value& value) {
        JSVM_Value result = nullptr;
        valueRef(value, &result);
        return result;
      });
  JSVM_Value func_value = nullptr;
  JSVMHelper::objectRef(f, &func_value);

  return JSVMHelper::callAsConstructor(this, func_value, converter,
                                       static_cast<int>(count));
}

Runtime::ScopeState* JSVMRuntime::pushScope() { return Runtime::pushScope(); }

void JSVMRuntime::popScope(ScopeState* state) { Runtime::popScope(state); }

bool JSVMRuntime::strictEquals(const piper::Symbol& a,
                               const piper::Symbol& b) const {
  JSVM_Value lhs = nullptr;
  JSVMHelper::symbolRef(a, &lhs);
  JSVM_Value rhs = nullptr;
  JSVMHelper::symbolRef(b, &rhs);

  bool result = false;
  JSVM_CALL_RETURN(OH_JSVM_StrictEquals, (getEnv(), lhs, rhs, &result), false);
  return result;
}

bool JSVMRuntime::strictEquals(const piper::String& a,
                               const piper::String& b) const {
  JSVM_Value lhs = nullptr;
  JSVMHelper::stringRef(a, &lhs);
  JSVM_Value rhs = nullptr;
  JSVMHelper::stringRef(b, &rhs);

  bool result = false;
  JSVM_CALL_RETURN(OH_JSVM_StrictEquals, (getEnv(), lhs, rhs, &result), false);
  return result;
}

bool JSVMRuntime::strictEquals(const piper::Object& a,
                               const piper::Object& b) const {
  JSVM_Value lhs = nullptr;
  JSVMHelper::objectRef(a, &lhs);
  JSVM_Value rhs = nullptr;
  JSVMHelper::objectRef(b, &rhs);

  bool result = false;
  JSVM_CALL_RETURN(OH_JSVM_StrictEquals, (getEnv(), lhs, rhs, &result), false);
  return result;
}

bool JSVMRuntime::instanceOf(const piper::Object& o, const piper::Function& f) {
  JSVM_Value obj_value = nullptr;
  JSVMHelper::objectRef(o, &obj_value);
  JSVM_Value ctor_value = nullptr;
  JSVMHelper::objectRef(f, &ctor_value);
  bool result = false;
  JSVM_CALL_RETURN(OH_JSVM_Instanceof,
                   (getEnv(), obj_value, ctor_value, &result), result);
  return result;
}

void JSVMRuntime::RequestGC() {
  LOGI("RequestGC");
  JSVM_CALL(OH_JSVM_MemoryPressureNotification,
            (getEnv(),
             JSVM_MemoryPressureLevel::JSVM_MEMORY_PRESSURE_LEVEL_CRITICAL));
}

void JSVMRuntime::InitInspector(
    const std::shared_ptr<InspectorRuntimeObserverNG>& observer) {
  // TODO
}

void JSVMRuntime::DestroyInspector() {
  // TODO
}
}  // namespace piper
}  // namespace lynx
