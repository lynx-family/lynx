// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "core/runtime/bindings/napi/worklet/napi_lepus_gesture.h"

#include <vector>
#include <utility>

#include "base/include/vector.h"

#include "base/include/log/logging.h"
#include "core/renderer/worklet/lepus_gesture.h"
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
const uint64_t kLepusGestureClassID = reinterpret_cast<uint64_t>(&kLepusGestureClassID);
const uint64_t kLepusGestureConstructorID = reinterpret_cast<uint64_t>(&kLepusGestureConstructorID);

using Wrapped = binding::NapiBaseWrapped<NapiLepusGesture>;
typedef Value (NapiLepusGesture::*InstanceCallback)(const CallbackInfo& info);
typedef void (NapiLepusGesture::*InstanceSetterCallback)(const CallbackInfo& info, const Value& value);

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

NapiLepusGesture::NapiLepusGesture(const CallbackInfo& info, bool skip_init_as_base)
    : NapiBridge(info) {
  set_type_id((void*)&kLepusGestureClassID);

  // If this is a base class or created by native, skip initialization since
  // impl side needs to have control over the construction of the impl object.
  if (skip_init_as_base || (info.Length() == 1 && info[0].IsExternal())) {
    return;
  }
  ExceptionMessage::IllegalConstructor(info.Env(), InterfaceName());
  return;
}

LepusGesture* NapiLepusGesture::ToImplUnsafe() {
  return impl_.get();
}

// static
Object NapiLepusGesture::Wrap(std::unique_ptr<LepusGesture> impl, Napi::Env env) {
  DCHECK(impl);
  auto obj = Constructor(env).New({Napi::External::New(env, nullptr, nullptr, nullptr)});
  ObjectWrap<NapiLepusGesture>::Unwrap(obj)->Init(std::move(impl));
  return obj;
}

// static
bool NapiLepusGesture::IsInstance(Napi::ScriptWrappable* wrappable) {
  if (!wrappable) {
    return false;
  }
  if (static_cast<NapiLepusGesture*>(wrappable)->type_id() == &kLepusGestureClassID) {
    return true;
  }
  return false;
}

void NapiLepusGesture::Init(std::unique_ptr<LepusGesture> impl) {
  DCHECK(impl);
  DCHECK(!impl_);

  impl_ = std::move(impl);
  // We only associate and call OnWrapped() once, when we init the root base.
  impl_->AssociateWithWrapper(this);
}

Value NapiLepusGesture::ActiveMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "Active", "1");
    return Value();
  }

  auto arg0_gestureId = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  impl_->Active(arg0_gestureId);
  return info.Env().Undefined();
}

Value NapiLepusGesture::FailMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "Fail", "1");
    return Value();
  }

  auto arg0_gestureId = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  impl_->Fail(arg0_gestureId);
  return info.Env().Undefined();
}

Value NapiLepusGesture::EndMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "End", "1");
    return Value();
  }

  auto arg0_gestureId = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  impl_->End(arg0_gestureId);
  return info.Env().Undefined();
}

Value NapiLepusGesture::ScrollByMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 2) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "ScrollBy", "2");
    return Value();
  }

  auto arg0_deltaX = NativeValueTraits<IDLFloat>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto arg1_deltaY = NativeValueTraits<IDLFloat>::NativeValue(info, 1);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto&& result = impl_->ScrollBy(arg0_deltaX, arg1_deltaY);
  return result;
}

// static
Napi::Class* NapiLepusGesture::Class(Napi::Env env) {
  auto* clazz = env.GetInstanceData<Napi::Class>(kLepusGestureClassID);
  if (clazz) {
    return clazz;
  }

  base::InlineVector<Wrapped::PropertyDescriptor, 4> props;

  // Attributes

  // Methods
  AddInstanceMethod(props, "active", &NapiLepusGesture::ActiveMethod);
  AddInstanceMethod(props, "fail", &NapiLepusGesture::FailMethod);
  AddInstanceMethod(props, "end", &NapiLepusGesture::EndMethod);
  AddInstanceMethod(props, "scrollBy", &NapiLepusGesture::ScrollByMethod);

  // Cache the class
  clazz = new Napi::Class(Wrapped::DefineClass(env, "LepusGesture", props.size(), props.data<const napi_property_descriptor>()));
  env.SetInstanceData<Napi::Class>(kLepusGestureClassID, clazz);
  return clazz;
}

// static
Function NapiLepusGesture::Constructor(Napi::Env env) {
  FunctionReference* ref = env.GetInstanceData<FunctionReference>(kLepusGestureConstructorID);
  if (ref) {
    return ref->Value();
  }

  // Cache the constructor for future use
  ref = new FunctionReference();
  ref->Reset(Class(env)->Get(env), 1);
  env.SetInstanceData<FunctionReference>(kLepusGestureConstructorID, ref);
  return ref->Value();
}

// static
void NapiLepusGesture::Install(Napi::Env env, Object& target) {
  if (target.Has("LepusGesture").FromMaybe(false)) {
    return;
  }
  target.Set("LepusGesture", Constructor(env));
}

}  // namespace worklet
}  // namespace lynx
