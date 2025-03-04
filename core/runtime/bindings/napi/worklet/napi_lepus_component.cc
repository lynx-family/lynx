// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "core/runtime/bindings/napi/worklet/napi_lepus_component.h"

#include <vector>
#include <utility>

#include "base/include/vector.h"

#include "base/include/log/logging.h"
#include "core/renderer/worklet/lepus_component.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/runtime/bindings/napi/worklet/napi_frame_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_func_callback.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_element.h"
#include "third_party/binding/napi/exception_message.h"
#include "third_party/binding/napi/napi_base_wrap.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

using Napi::Array;
using Napi::CallbackInfo;
using Napi::Error;
using Napi::Function;
using Napi::FunctionReference;
using Napi::Number;
using Napi::Object;
using Napi::ObjectWrap;
using Napi::String;
using Napi::TypeError;
using Napi::Value;

using Napi::ArrayBuffer;
using Napi::Int8Array;
using Napi::Uint8Array;
using Napi::Int16Array;
using Napi::Uint16Array;
using Napi::Int32Array;
using Napi::Uint32Array;
using Napi::Float32Array;
using Napi::Float64Array;
using Napi::DataView;

using lynx::binding::IDLBoolean;
using lynx::binding::IDLDouble;
using lynx::binding::IDLFloat;
using lynx::binding::IDLFunction;
using lynx::binding::IDLNumber;
using lynx::binding::IDLString;
using lynx::binding::IDLUnrestrictedFloat;
using lynx::binding::IDLUnrestrictedDouble;
using lynx::binding::IDLNullable;
using lynx::binding::IDLObject;
using lynx::binding::IDLInt8Array;
using lynx::binding::IDLInt16Array;
using lynx::binding::IDLInt32Array;
using lynx::binding::IDLUint8ClampedArray;
using lynx::binding::IDLUint8Array;
using lynx::binding::IDLUint16Array;
using lynx::binding::IDLUint32Array;
using lynx::binding::IDLFloat32Array;
using lynx::binding::IDLFloat64Array;
using lynx::binding::IDLArrayBuffer;
using lynx::binding::IDLArrayBufferView;
using lynx::binding::IDLDictionary;
using lynx::binding::IDLSequence;
using lynx::binding::NativeValueTraits;

using lynx::binding::ExceptionMessage;

namespace lynx {
namespace worklet {

namespace {
const uint64_t kLepusComponentClassID = reinterpret_cast<uint64_t>(&kLepusComponentClassID);
const uint64_t kLepusComponentConstructorID = reinterpret_cast<uint64_t>(&kLepusComponentConstructorID);

using Wrapped = binding::NapiBaseWrapped<NapiLepusComponent>;
typedef Value (NapiLepusComponent::*InstanceCallback)(const CallbackInfo& info);
typedef void (NapiLepusComponent::*InstanceSetterCallback)(const CallbackInfo& info, const Value& value);

__attribute__((unused))
void AddAttribute(base::Vector<Wrapped::PropertyDescriptor>& props,
                  const char* name,
                  InstanceCallback getter,
                  InstanceSetterCallback setter) {
  props.push_back(
      Wrapped::InstanceAccessor(name, getter, setter, napi_default_jsproperty));
}

__attribute__((unused))
void AddInstanceMethod(base::Vector<Wrapped::PropertyDescriptor>& props,
                       const char* name,
                       InstanceCallback method) {
  props.push_back(
      Wrapped::InstanceMethod(name, method, napi_default_jsproperty));
}
}  // namespace

NapiLepusComponent::NapiLepusComponent(const CallbackInfo& info, bool skip_init_as_base)
    : NapiBridge(info) {
  set_type_id((void*)&kLepusComponentClassID);

  // If this is a base class or created by native, skip initialization since
  // impl side needs to have control over the construction of the impl object.
  if (skip_init_as_base || (info.Length() == 1 && info[0].IsExternal())) {
    return;
  }
  ExceptionMessage::IllegalConstructor(info.Env(), InterfaceName());
  return;
}

LepusComponent* NapiLepusComponent::ToImplUnsafe() {
  return impl_.get();
}

// static
Object NapiLepusComponent::Wrap(std::unique_ptr<LepusComponent> impl, Napi::Env env) {
  DCHECK(impl);
  auto obj = Constructor(env).New({Napi::External::New(env, nullptr, nullptr, nullptr)});
  ObjectWrap<NapiLepusComponent>::Unwrap(obj)->Init(std::move(impl));
  return obj;
}

// static
bool NapiLepusComponent::IsInstance(Napi::ScriptWrappable* wrappable) {
  if (!wrappable) {
    return false;
  }
  if (static_cast<NapiLepusComponent*>(wrappable)->type_id() == &kLepusComponentClassID) {
    return true;
  }
  return false;
}

void NapiLepusComponent::Init(std::unique_ptr<LepusComponent> impl) {
  DCHECK(impl);
  DCHECK(!impl_);

  impl_ = std::move(impl);
  // We only associate and call OnWrapped() once, when we init the root base.
  impl_->AssociateWithWrapper(this);
}

Value NapiLepusComponent::QuerySelectorMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "QuerySelector", "1");
    return Value();
  }

  auto arg0_selector = NativeValueTraits<IDLString>::NativeValue(info, 0);

  auto&& result = impl_->QuerySelector(std::move(arg0_selector));
  return result ? (result->IsNapiWrapped() ? result->NapiObject() : NapiLepusElement::Wrap(std::unique_ptr<LepusElement>(std::move(result)), info.Env())) : info.Env().Null();
}

Value NapiLepusComponent::QuerySelectorAllMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "QuerySelectorAll", "1");
    return Value();
  }

  auto arg0_selector = NativeValueTraits<IDLString>::NativeValue(info, 0);

  const auto& vector_result = impl_->QuerySelectorAll(std::move(arg0_selector));
  auto result = Array::New(info.Env(), vector_result.size());
  for (size_t i = 0; i < vector_result.size(); ++i) {
    result[static_cast<uint32_t>(i)] = (vector_result[i]->IsNapiWrapped() ? vector_result[i]->NapiObject() : NapiLepusElement::Wrap(std::unique_ptr<LepusElement>(std::move(vector_result[i])), info.Env()));
  }
  return result;
}

Value NapiLepusComponent::RequestAnimationFrameMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "RequestAnimationFrame", "1");
    return Value();
  }

  auto arg0_cb = NativeValueTraits<IDLFunction<NapiFrameCallback>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto&& result = impl_->RequestAnimationFrame(std::move(arg0_cb));
  return Number::New(info.Env(), result);
}

Value NapiLepusComponent::CancelAnimationFrameMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "CancelAnimationFrame", "1");
    return Value();
  }

  auto arg0_id = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  impl_->CancelAnimationFrame(arg0_id);
  return info.Env().Undefined();
}

Value NapiLepusComponent::TriggerEventMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 3) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "TriggerEvent", "3");
    return Value();
  }

  auto arg0_eventName = NativeValueTraits<IDLString>::NativeValue(info, 0);

  auto arg1_eventDetail = NativeValueTraits<IDLObject>::NativeValue(info, 1);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  auto arg2_eventOption = NativeValueTraits<IDLObject>::NativeValue(info, 2);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->TriggerEvent(std::move(arg0_eventName), arg1_eventDetail, arg2_eventOption);
  return info.Env().Undefined();
}

Value NapiLepusComponent::GetStoreMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  auto&& result = impl_->GetStore();
  return result;
}

Value NapiLepusComponent::SetStoreMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "SetStore", "1");
    return Value();
  }

  auto arg0_data = NativeValueTraits<IDLObject>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->SetStore(arg0_data);
  return info.Env().Undefined();
}

Value NapiLepusComponent::GetDataMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  auto&& result = impl_->GetData();
  return result;
}

Value NapiLepusComponent::SetDataMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "SetData", "1");
    return Value();
  }

  auto arg0_data = NativeValueTraits<IDLObject>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->SetData(arg0_data);
  return info.Env().Undefined();
}

Value NapiLepusComponent::GetPropertiesMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  auto&& result = impl_->GetProperties();
  return result;
}

Value NapiLepusComponent::CallJSFunctionMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 2) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "CallJSFunction", "2");
    return Value();
  }

  auto arg0_methodName = NativeValueTraits<IDLString>::NativeValue(info, 0);

  auto arg1_methodParam = NativeValueTraits<IDLObject>::NativeValue(info, 1);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  if (info.Length() <= 2) {
    impl_->CallJSFunction(std::move(arg0_methodName), arg1_methodParam);
    return info.Env().Undefined();
  }

  auto arg2_cb = NativeValueTraits<IDLFunction<NapiFuncCallback>>::NativeValue(info, 2);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->CallJSFunction(std::move(arg0_methodName), arg1_methodParam, std::move(arg2_cb));
  return info.Env().Undefined();
}

// static
Napi::Class* NapiLepusComponent::Class(Napi::Env env) {
  auto* clazz = env.GetInstanceData<Napi::Class>(kLepusComponentClassID);
  if (clazz) {
    return clazz;
  }

  base::InlineVector<Wrapped::PropertyDescriptor, 11> props;

  // Attributes

  // Methods
  AddInstanceMethod(props, "querySelector", &NapiLepusComponent::QuerySelectorMethod);
  AddInstanceMethod(props, "querySelectorAll", &NapiLepusComponent::QuerySelectorAllMethod);
  AddInstanceMethod(props, "requestAnimationFrame", &NapiLepusComponent::RequestAnimationFrameMethod);
  AddInstanceMethod(props, "cancelAnimationFrame", &NapiLepusComponent::CancelAnimationFrameMethod);
  AddInstanceMethod(props, "triggerEvent", &NapiLepusComponent::TriggerEventMethod);
  AddInstanceMethod(props, "getStore", &NapiLepusComponent::GetStoreMethod);
  AddInstanceMethod(props, "setStore", &NapiLepusComponent::SetStoreMethod);
  AddInstanceMethod(props, "getData", &NapiLepusComponent::GetDataMethod);
  AddInstanceMethod(props, "setData", &NapiLepusComponent::SetDataMethod);
  AddInstanceMethod(props, "getProperties", &NapiLepusComponent::GetPropertiesMethod);
  AddInstanceMethod(props, "callJSFunction", &NapiLepusComponent::CallJSFunctionMethod);

  // Cache the class
  clazz = new Napi::Class(Wrapped::DefineClass(env, "LepusComponent", props.size(), props.data<const napi_property_descriptor>()));
  env.SetInstanceData<Napi::Class>(kLepusComponentClassID, clazz);
  return clazz;
}

// static
Function NapiLepusComponent::Constructor(Napi::Env env) {
  FunctionReference* ref = env.GetInstanceData<FunctionReference>(kLepusComponentConstructorID);
  if (ref) {
    return ref->Value();
  }

  // Cache the constructor for future use
  ref = new FunctionReference();
  ref->Reset(Class(env)->Get(env), 1);
  env.SetInstanceData<FunctionReference>(kLepusComponentConstructorID, ref);
  return ref->Value();
}

// static
void NapiLepusComponent::Install(Napi::Env env, Object& target) {
  if (target.Has("LepusComponent").FromMaybe(false)) {
    return;
  }
  target.Set("LepusComponent", Constructor(env));
}

}  // namespace worklet
}  // namespace lynx
