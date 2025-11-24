// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_HELPER_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_HELPER_H_

#include <ark_runtime/jsvm_types.h>

#include <string>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSVMRuntime;
namespace detail {
class JSVMSymbolValue final : public Runtime::PointerValue {
 public:
  BASE_DISALLOW_COPY_AND_ASSIGN(JSVMSymbolValue);
  JSVMSymbolValue(JSVMRuntime* rt, JSVM_Value sym_val);
  void invalidate() override;
  std::string Name() override { return "JSVMSymbolValue"; }

  JSVMRuntime* rt_;
  JSVM_Ref sym_ref_;
};

class JSVMStringValue final : public Runtime::PointerValue {
 public:
  BASE_DISALLOW_COPY_AND_ASSIGN(JSVMStringValue);
  JSVMStringValue(JSVMRuntime* rt, JSVM_Value str_val);
  void invalidate() override;
  std::string Name() override { return "JSVMStringValue"; }

  JSVMRuntime* rt_;
  JSVM_Ref str_ref_;
};

class JSVMObjectValue final : public Runtime::PointerValue {
 public:
  JSVMObjectValue(JSVMRuntime* rt, JSVM_Value obj_val);
  void invalidate() override;
  std::string Name() override { return "JSVMObjectValue"; }

  JSVMRuntime* rt_;
  JSVM_Ref obj_ref_;
};

class JSVMHelper {
 public:
  static piper::Value createValue(JSVM_Value value, JSVMRuntime* rt);

  static void symbolRef(const piper::Symbol& sym, JSVM_Value* value);

  static void stringRef(const piper::String& str, JSVM_Value* value);
  static void stringRef(const piper::PropNameID& sym, JSVM_Value* value);

  static void objectRef(const piper::Object& obj, JSVM_Value* value);

  static std::string JSStringToSTLString(JSVM_Value s, JSVMRuntime* rt);
  // Factory methods for creating String/Object
  static piper::Symbol createSymbol(JSVM_Value sym, JSVMRuntime* rt);
  static piper::String createString(JSVM_Value str, JSVMRuntime* rt);
  static piper::PropNameID createPropNameID(JSVM_Value value, JSVMRuntime* rt);
  static piper::Object createObject(JSVMRuntime* rt);
  static piper::Object createObject(JSVM_Value obj, JSVMRuntime* rt);

  // Used by factory methods and clone methods
  static piper::Runtime::PointerValue* makeSymbolValue(JSVM_Value sym_val,
                                                       JSVMRuntime* rt);
  static piper::Runtime::PointerValue* makeStringValue(JSVM_Value str_val,
                                                       JSVMRuntime* rt);
  static piper::Runtime::PointerValue* makeObjectValue(JSVM_Value obj_val,
                                                       JSVMRuntime* rt);

  static std::optional<piper::Value> call(piper::JSVMRuntime* rt,
                                          const piper::Function& f,
                                          const piper::Object& jsThis,
                                          JSVM_Value* args, size_t nArgs);

  static std::optional<piper::Value> callAsConstructor(piper::JSVMRuntime* rt,
                                                       JSVM_Value obj,
                                                       JSVM_Value* args,
                                                       int nArgs);

  static void ConvertToJSVMString(JSVMRuntime* rt, const std::string& s,
                                  JSVM_Value* value);

  static void ThrowJsException(JSVMRuntime* rt,
                               const std::string& error_message,
                               const std::string& error_stack);

  static void EnableInspector(JSVMRuntime* rt, bool break_next_line);

  static void CloseInspector(JSVMRuntime* rt);
};

}  // namespace detail
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_HELPER_H_
