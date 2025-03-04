// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "core/runtime/bindings/napi/worklet/napi_lepus_lynx.h"

#include <vector>
#include <utility>

#include "base/include/vector.h"

#include "base/include/log/logging.h"
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_lynx.h"
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
const uint64_t kLepusLynxClassID = reinterpret_cast<uint64_t>(&kLepusLynxClassID);
const uint64_t kLepusLynxConstructorID = reinterpret_cast<uint64_t>(&kLepusLynxConstructorID);

using Wrapped = binding::NapiBaseWrapped<NapiLepusLynx>;
typedef Value (NapiLepusLynx::*InstanceCallback)(const CallbackInfo& info);
typedef void (NapiLepusLynx::*InstanceSetterCallback)(const CallbackInfo& info, const Value& value);

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

NapiLepusLynx::NapiLepusLynx(const CallbackInfo& info, bool skip_init_as_base)
    : NapiBridge(info) {
  set_type_id((void*)&kLepusLynxClassID);

  // If this is a base class or created by native, skip initialization since
  // impl side needs to have control over the construction of the impl object.
  if (skip_init_as_base || (info.Length() == 1 && info[0].IsExternal())) {
    return;
  }
  ExceptionMessage::IllegalConstructor(info.Env(), InterfaceName());
  return;
}

LepusLynx* NapiLepusLynx::ToImplUnsafe() {
  return impl_.get();
}

// static
Object NapiLepusLynx::Wrap(std::unique_ptr<LepusLynx> impl, Napi::Env env) {
  DCHECK(impl);
  auto obj = Constructor(env).New({Napi::External::New(env, nullptr, nullptr, nullptr)});
  ObjectWrap<NapiLepusLynx>::Unwrap(obj)->Init(std::move(impl));
  return obj;
}

// static
bool NapiLepusLynx::IsInstance(Napi::ScriptWrappable* wrappable) {
  if (!wrappable) {
    return false;
  }
  if (static_cast<NapiLepusLynx*>(wrappable)->type_id() == &kLepusLynxClassID) {
    return true;
  }
  return false;
}

void NapiLepusLynx::Init(std::unique_ptr<LepusLynx> impl) {
  DCHECK(impl);
  DCHECK(!impl_);

  impl_ = std::move(impl);
  // We only associate and call OnWrapped() once, when we init the root base.
  impl_->AssociateWithWrapper(this);
}

Value NapiLepusLynx::TriggerLepusBridgeMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 3) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "TriggerLepusBridge", "3");
    return Value();
  }

  auto arg0_methodName = NativeValueTraits<IDLString>::NativeValue(info, 0);

  auto arg1_methodDetail = NativeValueTraits<IDLObject>::NativeValue(info, 1);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  auto arg2_cb = NativeValueTraits<IDLFunction<NapiFuncCallback>>::NativeValue(info, 2);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->TriggerLepusBridge(std::move(arg0_methodName), arg1_methodDetail, std::move(arg2_cb));
  return info.Env().Undefined();
}

Value NapiLepusLynx::TriggerLepusBridgeSyncMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 2) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "TriggerLepusBridgeSync", "2");
    return Value();
  }

  auto arg0_methodName = NativeValueTraits<IDLString>::NativeValue(info, 0);

  auto arg1_methodDetail = NativeValueTraits<IDLObject>::NativeValue(info, 1);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto&& result = impl_->TriggerLepusBridgeSync(std::move(arg0_methodName), arg1_methodDetail);
  return result;
}

Value NapiLepusLynx::SetTimeoutMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 2) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "SetTimeout", "2");
    return Value();
  }

  auto arg0_cb = NativeValueTraits<IDLFunction<NapiFuncCallback>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto arg1_delay = NativeValueTraits<IDLNumber>::NativeValue(info, 1);

  auto&& result = impl_->SetTimeout(std::move(arg0_cb), arg1_delay);
  return Number::New(info.Env(), result);
}

Value NapiLepusLynx::ClearTimeoutMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "ClearTimeout", "1");
    return Value();
  }

  auto arg0_id = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  impl_->ClearTimeout(arg0_id);
  return info.Env().Undefined();
}

Value NapiLepusLynx::SetIntervalMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 2) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "SetInterval", "2");
    return Value();
  }

  auto arg0_cb = NativeValueTraits<IDLFunction<NapiFuncCallback>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return Value();
  }

  auto arg1_delay = NativeValueTraits<IDLNumber>::NativeValue(info, 1);

  auto&& result = impl_->SetInterval(std::move(arg0_cb), arg1_delay);
  return Number::New(info.Env(), result);
}

Value NapiLepusLynx::ClearIntervalMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "ClearInterval", "1");
    return Value();
  }

  auto arg0_id = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  impl_->ClearInterval(arg0_id);
  return info.Env().Undefined();
}

// static
Napi::Class* NapiLepusLynx::Class(Napi::Env env) {
  auto* clazz = env.GetInstanceData<Napi::Class>(kLepusLynxClassID);
  if (clazz) {
    return clazz;
  }

  base::InlineVector<Wrapped::PropertyDescriptor, 6> props;

  // Attributes

  // Methods
  AddInstanceMethod(props, "triggerLepusBridge", &NapiLepusLynx::TriggerLepusBridgeMethod);
  AddInstanceMethod(props, "triggerLepusBridgeSync", &NapiLepusLynx::TriggerLepusBridgeSyncMethod);
  AddInstanceMethod(props, "setTimeout", &NapiLepusLynx::SetTimeoutMethod);
  AddInstanceMethod(props, "clearTimeout", &NapiLepusLynx::ClearTimeoutMethod);
  AddInstanceMethod(props, "setInterval", &NapiLepusLynx::SetIntervalMethod);
  AddInstanceMethod(props, "clearInterval", &NapiLepusLynx::ClearIntervalMethod);

  // Cache the class
  clazz = new Napi::Class(Wrapped::DefineClass(env, "LepusLynx", props.size(), props.data<const napi_property_descriptor>()));
  env.SetInstanceData<Napi::Class>(kLepusLynxClassID, clazz);
  return clazz;
}

// static
Function NapiLepusLynx::Constructor(Napi::Env env) {
  FunctionReference* ref = env.GetInstanceData<FunctionReference>(kLepusLynxConstructorID);
  if (ref) {
    return ref->Value();
  }

  // Cache the constructor for future use
  ref = new FunctionReference();
  ref->Reset(Class(env)->Get(env), 1);
  env.SetInstanceData<FunctionReference>(kLepusLynxConstructorID, ref);
  return ref->Value();
}

// static
void NapiLepusLynx::Install(Napi::Env env, Object& target) {
  if (target.Has("LepusLynx").FromMaybe(false)) {
    return;
  }
  target.Set("LepusLynx", Constructor(env));
}

}  // namespace worklet
}  // namespace lynx
