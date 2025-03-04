// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.h.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#ifndef JSBRIDGE_BINDINGS_GEN_TEST_NAPI_TEST_ELEMENT_H_
#define JSBRIDGE_BINDINGS_GEN_TEST_NAPI_TEST_ELEMENT_H_

#include <memory>

#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/native_value_traits.h"

namespace lynx {
namespace gen_test {

using binding::NapiBridge;
using binding::ImplBase;

class TestElement;

class NapiTestElement : public NapiBridge {
 public:
  NapiTestElement(const Napi::CallbackInfo&, bool skip_init_as_base = false);
  ~NapiTestElement() override;

  TestElement* ToImplUnsafe();

  static Napi::Object Wrap(std::unique_ptr<TestElement>, Napi::Env);
  static bool IsInstance(Napi::ScriptWrappable*);

  void Init(std::unique_ptr<TestElement>);

  // Attributes

  // Methods
  Napi::Value GetContextMethod(const Napi::CallbackInfo&);

  // Overload Hubs

  // Overloads

  // Injection hook
  static void Install(Napi::Env, Napi::Object&);

  static Napi::Function Constructor(Napi::Env);
  static Napi::Class* Class(Napi::Env);

  // Interface name
  static constexpr const char* InterfaceName() {
    return "TestElement";
  }

 private:
  void Init(const Napi::CallbackInfo&);
  std::unique_ptr<TestElement> impl_;
  // The unique id of this object in JS command buffer.
  uint32_t object_id_;
};

}  // namespace gen_test
}  // namespace lynx

#endif  // JSBRIDGE_BINDINGS_GEN_TEST_NAPI_TEST_ELEMENT_H_
