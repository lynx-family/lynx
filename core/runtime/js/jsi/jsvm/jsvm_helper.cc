// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_helper.h"

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

#include <optional>
#include <string>

#include "base/include/log/logging.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime.h"
#include "core/runtime/js/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace runtime {
namespace js {
namespace detail {
static constexpr int16_t kInspectorPort = 9225;
static constexpr char kInspectorHost[] = "localhost";

JSVMSymbolValue::JSVMSymbolValue(JSVMRuntime* rt, JSVM_Value sym_val)
    : rt_(rt) {
  JSVM_CALL(rt_, OH_JSVM_CreateReference, rt_->getEnv(), sym_val, 1, &sym_ref_);
}

void JSVMSymbolValue::invalidate() {
  if (sym_ref_) {
    uint32_t result = 0;
    JSVM_CALL(rt_, OH_JSVM_ReferenceUnref, rt_->getEnv(), sym_ref_, &result);
  }
  delete this;
}

JSVMStringValue::JSVMStringValue(JSVMRuntime* rt, JSVM_Value str_val)
    : rt_(rt) {
  JSVM_CALL(rt_, OH_JSVM_CreateReference, rt_->getEnv(), str_val, 1, &str_ref_);
}

void JSVMStringValue::invalidate() {
  if (str_ref_) {
    uint32_t result = 0;
    JSVM_CALL(rt_, OH_JSVM_ReferenceUnref, rt_->getEnv(), str_ref_, &result);
  }
  delete this;
}

JSVMObjectValue::JSVMObjectValue(JSVMRuntime* rt, JSVM_Value obj_val)
    : rt_(rt) {
  JSVM_CALL(rt_, OH_JSVM_CreateReference, rt_->getEnv(), obj_val, 1, &obj_ref_);
};

void JSVMObjectValue::invalidate() {
  if (obj_ref_) {
    uint32_t result = 0;
    JSVM_CALL(rt_, OH_JSVM_ReferenceUnref, rt_->getEnv(), obj_ref_, &result);
  }
  delete this;
}

Value JSVMHelper::createValue(JSVM_Value value, JSVMRuntime* rt) {
  JSVM_ValueType type;

  JSVM_CALL_RETURN(rt, OH_JSVM_Typeof, Value(), rt->getEnv(), value, &type);
  if (type == JSVM_ValueType::JSVM_NUMBER) {
    double result;
    JSVM_CALL_RETURN(rt, OH_JSVM_GetValueDouble, Value(), rt->getEnv(), value,
                     &result);
    return Value(result);
  }
  if (type == JSVM_ValueType::JSVM_BIGINT) {
    int64_t result;
    bool lossless;
    JSVM_CALL_RETURN(rt, OH_JSVM_GetValueBigintInt64, Value(), rt->getEnv(),
                     value, &result, &lossless);
    return Value(static_cast<int>(result));
  }
  if (type == JSVM_ValueType::JSVM_BOOLEAN) {
    bool result;
    JSVM_CALL_RETURN(rt, OH_JSVM_GetValueBool, Value(), rt->getEnv(), value,
                     &result);
    return Value(result);
  }
  if (type == JSVM_ValueType::JSVM_NULL) {
    return Value(nullptr);
  }
  if (type == JSVM_ValueType::JSVM_UNDEFINED) {
    return Value();
  }
  if (type == JSVM_ValueType::JSVM_STRING) {
    auto result = Value(createString(value, rt));
    return result;
  }
  if (type == JSVM_ValueType::JSVM_OBJECT ||
      type == JSVM_ValueType::JSVM_FUNCTION) {
    auto result = Value(createObject(value, rt));
    return result;
  }
  if (type == JSVM_ValueType::JSVM_SYMBOL) {
    auto result = Value(createSymbol(value, rt));
    return result;
  }
  // WHAT ARE YOU
  abort();
}

void JSVMHelper::symbolRef(const Symbol& sym, JSVM_Value* value) {
  const JSVMSymbolValue* jsvm_sym =
      static_cast<const JSVMSymbolValue*>(Runtime::getPointerValue(sym));
  JSVM_CALL(jsvm_sym->rt_, OH_JSVM_GetReferenceValue, jsvm_sym->rt_->getEnv(),
            jsvm_sym->sym_ref_, value);
}

void JSVMHelper::stringRef(const String& str, JSVM_Value* value) {
  auto jsvm_str =
      static_cast<const JSVMStringValue*>(Runtime::getPointerValue(str));
  JSVM_CALL(jsvm_str->rt_, OH_JSVM_GetReferenceValue, jsvm_str->rt_->getEnv(),
            jsvm_str->str_ref_, value);
}

void JSVMHelper::stringRef(const PropNameID& sym, JSVM_Value* value) {
  auto jsvm_str =
      static_cast<const JSVMStringValue*>(Runtime::getPointerValue(sym));
  JSVM_CALL(jsvm_str->rt_, OH_JSVM_GetReferenceValue, jsvm_str->rt_->getEnv(),
            jsvm_str->str_ref_, value);
}

void JSVMHelper::objectRef(const Object& obj, JSVM_Value* value) {
  const JSVMObjectValue* jsvm_obj =
      static_cast<const JSVMObjectValue*>(Runtime::getPointerValue(obj));
  JSVM_CALL(jsvm_obj->rt_, OH_JSVM_GetReferenceValue, jsvm_obj->rt_->getEnv(),
            jsvm_obj->obj_ref_, value);
}

std::string JSVMHelper::JSStringToSTLString(JSVM_Value s, JSVMRuntime* rt) {
  size_t len = 0;
  JSVM_CALL_RETURN(rt, OH_JSVM_GetValueStringUtf8, std::string(), rt->getEnv(),
                   s, nullptr, JSVM_AUTO_LENGTH, &len);
  std::string output_str;
  output_str.resize(len + 1);
  JSVM_CALL_RETURN(rt, OH_JSVM_GetValueStringUtf8, std::string(), rt->getEnv(),
                   s, output_str.data(), output_str.size(), nullptr);
  output_str.resize(len);
  return output_str;
}

Symbol JSVMHelper::createSymbol(JSVM_Value sym, JSVMRuntime* rt) {
  return Runtime::make<Symbol>(makeSymbolValue(sym, rt));
}

String JSVMHelper::createString(JSVM_Value str, JSVMRuntime* rt) {
  return Runtime::make<String>(makeStringValue(str, rt));
}

PropNameID JSVMHelper::createPropNameID(JSVM_Value value, JSVMRuntime* rt) {
  JSVM_ValueType value_type = JSVM_ValueType::JSVM_UNDEFINED;
  JSVM_CALL(rt, OH_JSVM_Typeof, rt->getEnv(), value, &value_type);

  if (value_type == JSVM_ValueType::JSVM_STRING) {
    return Runtime::make<PropNameID>(makeStringValue(value, rt));
  }

  if (value_type == JSVM_ValueType::JSVM_SYMBOL) {
    return Runtime::make<PropNameID>(makeSymbolValue(value, rt));
  }
  abort();
}

Object JSVMHelper::createObject(JSVMRuntime* rt) {
  return createObject(nullptr, rt);
}

Object JSVMHelper::createObject(JSVM_Value obj, JSVMRuntime* rt) {
  return Runtime::make<Object>(makeObjectValue(obj, rt));
}

Runtime::PointerValue* JSVMHelper::makeSymbolValue(JSVM_Value sym_val,
                                                   JSVMRuntime* rt) {
  return new JSVMSymbolValue(rt, sym_val);
}

Runtime::PointerValue* JSVMHelper::makeStringValue(JSVM_Value str_val,
                                                   JSVMRuntime* rt) {
  if (str_val == nullptr) {
    JSVM_CALL(rt, OH_JSVM_CreateStringUtf8, rt->getEnv(), "", 0, &str_val);
  }
  return new JSVMStringValue(rt, str_val);
}

Runtime::PointerValue* JSVMHelper::makeObjectValue(JSVM_Value obj_val,
                                                   JSVMRuntime* rt) {
  if (obj_val == nullptr) {
    JSVM_CALL(rt, OH_JSVM_CreateObject, rt->getEnv(), &obj_val);
  }
  return new JSVMObjectValue(rt, obj_val);
}

std::optional<Value> JSVMHelper::call(JSVMRuntime* rt, const Function& f,
                                      const Object& jsThis, JSVM_Value* args,
                                      size_t nArgs) {
  HandleScopeWrapper scope(rt->getEnv());
  EnvHandleWrapper env_scope(rt->getEnv());
  JSVM_Value this_value = nullptr;
  objectRef(jsThis, &this_value);
  if (this_value == nullptr) {
    JSVM_CALL_RETURN(rt, OH_JSVM_GetGlobal, std::optional<Value>(),
                     rt->getEnv(), &this_value);
  }
  JSVM_Value func_vale = nullptr;
  objectRef(f, &func_vale);

  JSVM_Value result = nullptr;
  JSVM_CALL_RETURN(rt, OH_JSVM_CallFunction, std::optional<Value>(),
                   rt->getEnv(), this_value, func_vale, nArgs, args, &result);

  if (result == nullptr) {
    // This result will only be an empty handle when an exception occurred.
    // So this if-block should never be entered.
    // Here we make a IsEmpty() check just to ensure not crashing.
    // See:
    // https://chromium.googlesource.com/v8/v8.git/+/dc7926bebd49d8749074c414dcf08a846bae0007/src/api/api.cc#5337
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "Exception calling object as function. MaybeLocal empty."));
    return std::optional<Value>();
  }
  return createValue(result, rt);
}

std::optional<Value> JSVMHelper::callAsConstructor(JSVMRuntime* rt,
                                                   JSVM_Value obj,
                                                   JSVM_Value* args,
                                                   int nArgs) {
  // TODO(yangguangzhao.solace): Missing corresponding API
  return std::optional<Value>();
}

void JSVMHelper::ConvertToJSVMString(JSVMRuntime* rt, const std::string& s,
                                     JSVM_Value* value) {
  JSVM_CALL(rt, OH_JSVM_CreateStringUtf8, rt->getEnv(), s.c_str(), s.size(),
            value);
}

void JSVMHelper::ThrowJsException(JSVMRuntime* rt,
                                  const std::string& error_message,
                                  const std::string& error_stack) {
  JSVM_CALL(rt, OH_JSVM_ThrowError, rt->getEnv(), error_stack.c_str(),
            error_message.c_str());
}

void JSVMHelper::EnableInspector(JSVMRuntime* rt, bool break_next_line) {
  JSVM_CALL(rt, OH_JSVM_OpenInspector, rt->getEnv(), kInspectorHost,
            kInspectorPort);
  JSVM_CALL(rt, OH_JSVM_WaitForDebugger, rt->getEnv(), break_next_line);
}

void JSVMHelper::CloseInspector(JSVMRuntime* rt) {
  JSVM_CALL(rt, OH_JSVM_CloseInspector, rt->getEnv());
}
}  // namespace detail
}  // namespace js
}  // namespace runtime
}  // namespace lynx
