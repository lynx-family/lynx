// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "core/runtime/bindings/napi/worklet/napi_lepus_element.h"

#include <vector>
#include <utility>

#include "base/include/vector.h"

#include "base/include/log/logging.h"
#include "core/renderer/worklet/lepus_element.h"
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
const uint64_t kLepusElementClassID = reinterpret_cast<uint64_t>(&kLepusElementClassID);
const uint64_t kLepusElementConstructorID = reinterpret_cast<uint64_t>(&kLepusElementConstructorID);

using Wrapped = binding::NapiBaseWrapped<NapiLepusElement>;
typedef Value (NapiLepusElement::*InstanceCallback)(const CallbackInfo& info);
typedef void (NapiLepusElement::*InstanceSetterCallback)(const CallbackInfo& info, const Value& value);

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

NapiLepusElement::NapiLepusElement(const CallbackInfo& info, bool skip_init_as_base)
    : NapiBridge(info) {
  set_type_id((void*)&kLepusElementClassID);

  // If this is a base class or created by native, skip initialization since
  // impl side needs to have control over the construction of the impl object.
  if (skip_init_as_base || (info.Length() == 1 && info[0].IsExternal())) {
    return;
  }
  ExceptionMessage::IllegalConstructor(info.Env(), InterfaceName());
  return;
}

LepusElement* NapiLepusElement::ToImplUnsafe() {
  return impl_.get();
}

// static
Object NapiLepusElement::Wrap(std::unique_ptr<LepusElement> impl, Napi::Env env) {
  DCHECK(impl);
  auto obj = Constructor(env).New({Napi::External::New(env, nullptr, nullptr, nullptr)});
  ObjectWrap<NapiLepusElement>::Unwrap(obj)->Init(std::move(impl));
  return obj;
}

// static
bool NapiLepusElement::IsInstance(Napi::ScriptWrappable* wrappable) {
  if (!wrappable) {
    return false;
  }
  if (static_cast<NapiLepusElement*>(wrappable)->type_id() == &kLepusElementClassID) {
    return true;
  }
  return false;
}

void NapiLepusElement::Init(std::unique_ptr<LepusElement> impl) {
  DCHECK(impl);
  DCHECK(!impl_);

  impl_ = std::move(impl);
  // We only associate and call OnWrapped() once, when we init the root base.
  impl_->AssociateWithWrapper(this);
}

Value NapiLepusElement::SetAttributesMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "SetAttributes", "1");
    return Value();
  }

  auto arg0_attributes = NativeValueTraits<IDLObject>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->SetAttributes(arg0_attributes);
  return info.Env().Undefined();
}

Value NapiLepusElement::SetStylesMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "SetStyles", "1");
    return Value();
  }

  auto arg0_styles = NativeValueTraits<IDLObject>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->SetStyles(arg0_styles);
  return info.Env().Undefined();
}

Value NapiLepusElement::GetAttributesMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "GetAttributes", "1");
    return Value();
  }

  auto arg0_keys = NativeValueTraits<IDLSequence<IDLString>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto&& result = impl_->GetAttributes(std::move(arg0_keys));
  return result;
}

Value NapiLepusElement::GetComputedStylesMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "GetComputedStyles", "1");
    return Value();
  }

  auto arg0_keys = NativeValueTraits<IDLSequence<IDLString>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto&& result = impl_->GetComputedStyles(std::move(arg0_keys));
  return result;
}

Value NapiLepusElement::GetDatasetMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  auto&& result = impl_->GetDataset();
  return result;
}

Value NapiLepusElement::ScrollByMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 2) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "ScrollBy", "2");
    return Value();
  }

  auto arg0_width = NativeValueTraits<IDLFloat>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto arg1_height = NativeValueTraits<IDLFloat>::NativeValue(info, 1);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto&& result = impl_->ScrollBy(arg0_width, arg1_height);
  return result;
}

Value NapiLepusElement::GetBoundingClientRectMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  auto&& result = impl_->GetBoundingClientRect();
  return result;
}

Value NapiLepusElement::InvokeMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "Invoke", "1");
    return Value();
  }

  auto arg0_param = NativeValueTraits<IDLObject>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->Invoke(arg0_param);
  return info.Env().Undefined();
}

// static
Napi::Class* NapiLepusElement::Class(Napi::Env env) {
  auto* clazz = env.GetInstanceData<Napi::Class>(kLepusElementClassID);
  if (clazz) {
    return clazz;
  }

  base::InlineVector<Wrapped::PropertyDescriptor, 8> props;

  // Attributes

  // Methods
  AddInstanceMethod(props, "setAttributes", &NapiLepusElement::SetAttributesMethod);
  AddInstanceMethod(props, "setStyles", &NapiLepusElement::SetStylesMethod);
  AddInstanceMethod(props, "getAttributes", &NapiLepusElement::GetAttributesMethod);
  AddInstanceMethod(props, "getComputedStyles", &NapiLepusElement::GetComputedStylesMethod);
  AddInstanceMethod(props, "getDataset", &NapiLepusElement::GetDatasetMethod);
  AddInstanceMethod(props, "scrollBy", &NapiLepusElement::ScrollByMethod);
  AddInstanceMethod(props, "getBoundingClientRect", &NapiLepusElement::GetBoundingClientRectMethod);
  AddInstanceMethod(props, "invoke", &NapiLepusElement::InvokeMethod);

  // Cache the class
  clazz = new Napi::Class(Wrapped::DefineClass(env, "LepusElement", props.size(), props.data<const napi_property_descriptor>()));
  env.SetInstanceData<Napi::Class>(kLepusElementClassID, clazz);
  return clazz;
}

// static
Function NapiLepusElement::Constructor(Napi::Env env) {
  FunctionReference* ref = env.GetInstanceData<FunctionReference>(kLepusElementConstructorID);
  if (ref) {
    return ref->Value();
  }

  // Cache the constructor for future use
  ref = new FunctionReference();
  ref->Reset(Class(env)->Get(env), 1);
  env.SetInstanceData<FunctionReference>(kLepusElementConstructorID, ref);
  return ref->Value();
}

// static
void NapiLepusElement::Install(Napi::Env env, Object& target) {
  if (target.Has("LepusElement").FromMaybe(false)) {
    return;
  }
  target.Set("LepusElement", Constructor(env));
}

}  // namespace worklet
}  // namespace lynx
