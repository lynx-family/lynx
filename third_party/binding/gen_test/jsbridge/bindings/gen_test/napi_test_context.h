// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.h.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#ifndef JSBRIDGE_BINDINGS_GEN_TEST_NAPI_TEST_CONTEXT_H_
#define JSBRIDGE_BINDINGS_GEN_TEST_NAPI_TEST_CONTEXT_H_

#include <memory>

#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/native_value_traits.h"

namespace lynx {
namespace gen_test {

using binding::NapiBridge;
using binding::ImplBase;

class TestContext;

class NapiTestContext : public NapiBridge {
 public:
  NapiTestContext(const Napi::CallbackInfo&, bool skip_init_as_base = false);
  ~NapiTestContext() override;

  TestContext* ToImplUnsafe();

  static Napi::Object Wrap(std::unique_ptr<TestContext>, Napi::Env);
  static bool IsInstance(Napi::ScriptWrappable*);

  void Init(std::unique_ptr<TestContext>);

  // Attributes

  // Methods
  Napi::Value VoidFromStringMethod(const Napi::CallbackInfo&);
  Napi::Value StringFromVoidMethod(const Napi::CallbackInfo&);
  Napi::Value VoidFromStringArrayMethod(const Napi::CallbackInfo&);
  Napi::Value VoidFromTypedArrayMethod(const Napi::CallbackInfo&);
  Napi::Value VoidFromArrayBufferMethod(const Napi::CallbackInfo&);
  Napi::Value VoidFromArrayBufferViewMethod(const Napi::CallbackInfo&);
  Napi::Value VoidFromNullableArrayBufferViewMethod(const Napi::CallbackInfo&);
  Napi::Value FinishMethod(const Napi::CallbackInfo&);

  // Overload Hubs

  // Overloads

  // Injection hook
  static void Install(Napi::Env, Napi::Object&);

  static Napi::Function Constructor(Napi::Env);
  static Napi::Class* Class(Napi::Env);

  // Interface name
  static constexpr const char* InterfaceName() {
    return "TestContext";
  }

 private:
  ImplBase* ReleaseImpl();
  std::unique_ptr<TestContext> impl_;
  // The unique id of this object in JS command buffer.
  uint32_t object_id_;
};

}  // namespace gen_test
}  // namespace lynx

#endif  // JSBRIDGE_BINDINGS_GEN_TEST_NAPI_TEST_CONTEXT_H_
