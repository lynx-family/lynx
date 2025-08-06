// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/jsvm/jsvm_helper.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#include <optional>
#include <string>

#include "base/include/log/logging.h"
#include "core/runtime/jsi/jsvm/jsvm_runtime.h"
#include "core/runtime/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace piper {
namespace detail {
static constexpr int16_t kInspectorPort = 9225;
static constexpr char kInspectorHost[] = "localhost";

JSVMSymbolValue::JSVMSymbolValue(JSVM_Env env, JSVM_Value sym_val) : env_(env) {
  JSVM_CALL(OH_JSVM_CreateReference, (env, sym_val, 1, &sym_ref_));
}

void JSVMSymbolValue::invalidate() {
  if (sym_ref_) {
    uint32_t result = 0;
    JSVM_CALL(OH_JSVM_ReferenceUnref, (env_, sym_ref_, &result));
  }
  delete this;
}

JSVMStringValue::JSVMStringValue(JSVM_Env env, JSVM_Value str_val) : env_(env) {
  JSVM_CALL(OH_JSVM_CreateReference, (env, str_val, 1, &str_ref_));
}

void JSVMStringValue::invalidate() {
  if (str_ref_) {
    uint32_t result = 0;
    JSVM_CALL(OH_JSVM_ReferenceUnref, (env_, str_ref_, &result));
  }
  delete this;
}

JSVMObjectValue::JSVMObjectValue(JSVM_Env env, JSVM_Value obj_val) : env_(env) {
  JSVM_CALL(OH_JSVM_CreateReference, (env, obj_val, 1, &obj_ref_));
};

void JSVMObjectValue::invalidate() {
  if (obj_ref_) {
    uint32_t result = 0;
    JSVM_CALL(OH_JSVM_ReferenceUnref, (env_, obj_ref_, &result));
  }
  delete this;
}

piper::Value JSVMHelper::createValue(JSVM_Value value, JSVM_Env env) {
  JSVM_ValueType type;

  JSVM_CALL_RETURN(OH_JSVM_Typeof, (env, value, &type), piper::Value());
  if (type == JSVM_ValueType::JSVM_NUMBER) {
    double result;
    JSVM_CALL_RETURN(OH_JSVM_GetValueDouble, (env, value, &result),
                     piper::Value());
    return piper::Value(result);
  }
  if (type == JSVM_ValueType::JSVM_BIGINT) {
    int64_t result;
    bool lossless;
    JSVM_CALL_RETURN(OH_JSVM_GetValueBigintInt64,
                     (env, value, &result, &lossless), piper::Value());
    return piper::Value(static_cast<int>(result));
  }
  if (type == JSVM_ValueType::JSVM_BOOLEAN) {
    bool result;
    JSVM_CALL_RETURN(OH_JSVM_GetValueBool, (env, value, &result),
                     piper::Value());
    return piper::Value(result);
  }
  if (type == JSVM_ValueType::JSVM_NULL) {
    return piper::Value(nullptr);
  }
  if (type == JSVM_ValueType::JSVM_UNDEFINED) {
    return piper::Value();
  }
  if (type == JSVM_ValueType::JSVM_STRING) {
    auto result = piper::Value(createString(value, env));
    return result;
  }
  if (type == JSVM_ValueType::JSVM_OBJECT ||
      type == JSVM_ValueType::JSVM_FUNCTION) {
    auto result = piper::Value(createObject(value, env));
    return result;
  }
  if (type == JSVM_ValueType::JSVM_SYMBOL) {
    auto result = piper::Value(createSymbol(value, env));
    return result;
  }
  // WHAT ARE YOU
  abort();
}

void JSVMHelper::symbolRef(const piper::Symbol& sym, JSVM_Value* value) {
  const JSVMSymbolValue* jsvm_sym =
      static_cast<const JSVMSymbolValue*>(Runtime::getPointerValue(sym));
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (jsvm_sym->env_, jsvm_sym->sym_ref_, value));
}

void JSVMHelper::stringRef(const piper::String& str, JSVM_Value* value) {
  auto jsvm_str =
      static_cast<const JSVMStringValue*>(Runtime::getPointerValue(str));
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (jsvm_str->env_, jsvm_str->str_ref_, value));
}

void JSVMHelper::stringRef(const piper::PropNameID& sym, JSVM_Value* value) {
  auto jsvm_str =
      static_cast<const JSVMStringValue*>(Runtime::getPointerValue(sym));
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (jsvm_str->env_, jsvm_str->str_ref_, value));
}

void JSVMHelper::objectRef(const piper::Object& obj, JSVM_Value* value) {
  const JSVMObjectValue* jsvm_obj =
      static_cast<const JSVMObjectValue*>(Runtime::getPointerValue(obj));
  JSVM_CALL(OH_JSVM_GetReferenceValue,
            (jsvm_obj->env_, jsvm_obj->obj_ref_, value));
}

std::string JSVMHelper::JSStringToSTLString(JSVM_Value s, JSVM_Env env) {
  size_t len = 0;
  JSVM_CALL_RETURN(OH_JSVM_GetValueStringUtf8,
                   (env, s, nullptr, JSVM_AUTO_LENGTH, &len), std::string());
  std::string output_str;
  output_str.resize(len + 1);
  JSVM_CALL_RETURN(OH_JSVM_GetValueStringUtf8,
                   (env, s, output_str.data(), output_str.size(), nullptr),
                   std::string());
  return output_str.substr(0, len);
}

piper::Symbol JSVMHelper::createSymbol(JSVM_Value sym, JSVM_Env env) {
  return Runtime::make<piper::Symbol>(makeSymbolValue(sym, env));
}

piper::String JSVMHelper::createString(JSVM_Value str, JSVM_Env env) {
  return Runtime::make<piper::String>(makeStringValue(str, env));
}

piper::PropNameID JSVMHelper::createPropNameID(JSVM_Value value, JSVM_Env env) {
  JSVM_ValueType value_type;
  JSVM_CALL(OH_JSVM_Typeof, (env, value, &value_type));

  if (value_type == JSVM_ValueType::JSVM_STRING) {
    return Runtime::make<piper::PropNameID>(makeStringValue(value, env));
  }

  if (value_type == JSVM_ValueType::JSVM_SYMBOL) {
    return Runtime::make<piper::PropNameID>(makeSymbolValue(value, env));
  }
  abort();
}

piper::Object JSVMHelper::createObject(JSVM_Env env) {
  return createObject(nullptr, env);
}

piper::Object JSVMHelper::createObject(JSVM_Value obj, JSVM_Env env) {
  return Runtime::make<piper::Object>(makeObjectValue(obj, env));
}

piper::Runtime::PointerValue* JSVMHelper::makeSymbolValue(JSVM_Value sym_val,
                                                          JSVM_Env env) {
  return new JSVMSymbolValue(env, sym_val);
}

piper::Runtime::PointerValue* JSVMHelper::makeStringValue(JSVM_Value str_val,
                                                          JSVM_Env env) {
  if (str_val == nullptr) {
    JSVM_CALL(OH_JSVM_CreateStringUtf8, (env, "", 0, &str_val));
  }
  return new JSVMStringValue(env, str_val);
}

piper::Runtime::PointerValue* JSVMHelper::makeObjectValue(JSVM_Value obj_val,
                                                          JSVM_Env env) {
  if (obj_val == nullptr) {
    JSVM_CALL(OH_JSVM_CreateObject, (env, &obj_val));
  }
  return new JSVMObjectValue(env, obj_val);
}

std::optional<piper::Value> JSVMHelper::call(piper::JSVMRuntime* rt,
                                             const piper::Function& f,
                                             const piper::Object& jsThis,
                                             JSVM_Value* args, size_t nArgs) {
  HandleScopeWrapper scope(rt->getEnv());
  JSVM_Value this_value = nullptr;
  objectRef(jsThis, &this_value);
  if (this_value == nullptr) {
    JSVM_CALL_RETURN(OH_JSVM_GetGlobal, (rt->getEnv(), &this_value),
                     std::optional<piper::Value>());
  }
  JSVM_Value func_vale = nullptr;
  objectRef(f, &func_vale);

  JSVM_Value result = nullptr;
  JSVM_CALL_RETURN(OH_JSVM_CallFunction,
                   (rt->getEnv(), this_value, func_vale, nArgs, args, &result),
                   std::optional<piper::Value>());

  if (result == nullptr) {
    // This result will only be an empty handle when an exception occurred.
    // So this if-block should never be entered.
    // Here we make a IsEmpty() check just to ensure not crashing.
    // See:
    // https://chromium.googlesource.com/v8/v8.git/+/dc7926bebd49d8749074c414dcf08a846bae0007/src/api/api.cc#5337
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Exception calling object as function. MaybeLocal empty."));
    return std::optional<piper::Value>();
  }
  return createValue(result, rt->getEnv());
}

std::optional<piper::Value> JSVMHelper::callAsConstructor(
    piper::JSVMRuntime* rt, JSVM_Value obj, JSVM_Value* args, int nArgs) {
  // TODO(yangguangzhao.solace): Missing corresponding API
  return std::optional<piper::Value>();
}

void JSVMHelper::ConvertToJSVMString(JSVM_Env env, const std::string& s,
                                     JSVM_Value* value) {
  JSVM_CALL(OH_JSVM_CreateStringUtf8, (env, s.c_str(), s.size(), value));
}

void JSVMHelper::ThrowJsException(JSVM_Env env,
                                  const std::string& error_message,
                                  const std::string& error_stack) {
  JSVM_CALL(OH_JSVM_ThrowError,
            (env, error_stack.c_str(), error_message.c_str()));
}

void JSVMHelper::EnableInspector(JSVM_Env env, bool break_next_line) {
  JSVM_CALL(OH_JSVM_OpenInspector, (env, kInspectorHost, kInspectorPort));
  JSVM_CALL(OH_JSVM_WaitForDebugger, (env, break_next_line));
}

void JSVMHelper::CloseInspector(JSVM_Env env) {
  JSVM_CALL(OH_JSVM_CloseInspector, (env));
}
}  // namespace detail
}  // namespace piper
}  // namespace lynx
