// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.h.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#ifndef CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LEPUS_COMPONENT_H_
#define CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LEPUS_COMPONENT_H_

#include <memory>

#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/native_value_traits.h"

namespace lynx {
namespace worklet {

using binding::NapiBridge;
using binding::ImplBase;

class LepusComponent;

class NapiLepusComponent : public NapiBridge {
 public:
  NapiLepusComponent(const Napi::CallbackInfo&, bool skip_init_as_base = false);

  LepusComponent* ToImplUnsafe();

  static Napi::Object Wrap(std::unique_ptr<LepusComponent>, Napi::Env);
  static bool IsInstance(Napi::ScriptWrappable*);

  void Init(std::unique_ptr<LepusComponent>);

  // Attributes

  // Methods
  Napi::Value QuerySelectorMethod(const Napi::CallbackInfo&);
  Napi::Value QuerySelectorAllMethod(const Napi::CallbackInfo&);
  Napi::Value RequestAnimationFrameMethod(const Napi::CallbackInfo&);
  Napi::Value CancelAnimationFrameMethod(const Napi::CallbackInfo&);
  Napi::Value TriggerEventMethod(const Napi::CallbackInfo&);
  Napi::Value GetStoreMethod(const Napi::CallbackInfo&);
  Napi::Value SetStoreMethod(const Napi::CallbackInfo&);
  Napi::Value GetDataMethod(const Napi::CallbackInfo&);
  Napi::Value SetDataMethod(const Napi::CallbackInfo&);
  Napi::Value GetPropertiesMethod(const Napi::CallbackInfo&);
  Napi::Value CallJSFunctionMethod(const Napi::CallbackInfo&);

  // Overload Hubs

  // Overloads

  // Injection hook
  static void Install(Napi::Env, Napi::Object&);

  static Napi::Function Constructor(Napi::Env);
  static Napi::Class* Class(Napi::Env);

  // Interface name
  static constexpr const char* InterfaceName() {
    return "LepusComponent";
  }

 private:
  std::unique_ptr<LepusComponent> impl_;
};

}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LEPUS_COMPONENT_H_
