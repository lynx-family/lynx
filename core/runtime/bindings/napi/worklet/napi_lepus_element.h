// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.h.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#ifndef CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LEPUS_ELEMENT_H_
#define CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LEPUS_ELEMENT_H_

#include <memory>

#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/native_value_traits.h"

namespace lynx {
namespace worklet {

using binding::NapiBridge;
using binding::ImplBase;

class LepusElement;

class NapiLepusElement : public NapiBridge {
 public:
  NapiLepusElement(const Napi::CallbackInfo&, bool skip_init_as_base = false);

  LepusElement* ToImplUnsafe();

  static Napi::Object Wrap(std::unique_ptr<LepusElement>, Napi::Env);
  static bool IsInstance(Napi::ScriptWrappable*);

  void Init(std::unique_ptr<LepusElement>);

  // Attributes

  // Methods
  Napi::Value SetAttributesMethod(const Napi::CallbackInfo&);
  Napi::Value SetStylesMethod(const Napi::CallbackInfo&);
  Napi::Value GetAttributesMethod(const Napi::CallbackInfo&);
  Napi::Value GetComputedStylesMethod(const Napi::CallbackInfo&);
  Napi::Value GetDatasetMethod(const Napi::CallbackInfo&);
  Napi::Value ScrollByMethod(const Napi::CallbackInfo&);
  Napi::Value GetBoundingClientRectMethod(const Napi::CallbackInfo&);
  Napi::Value InvokeMethod(const Napi::CallbackInfo&);

  // Overload Hubs

  // Overloads

  // Injection hook
  static void Install(Napi::Env, Napi::Object&);

  static Napi::Function Constructor(Napi::Env);
  static Napi::Class* Class(Napi::Env);

  // Interface name
  static constexpr const char* InterfaceName() {
    return "LepusElement";
  }

 private:
  std::unique_ptr<LepusElement> impl_;
};

}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LEPUS_ELEMENT_H_
