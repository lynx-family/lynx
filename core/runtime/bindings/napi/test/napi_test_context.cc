// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "core/runtime/bindings/napi/test/napi_test_context.h"

#include <vector>
#include <utility>

#include "core/runtime/bindings/napi/test/test_context.h"
#include "base/include/log/logging.h"
#include "third_party/binding/napi/exception_message.h"
#include "third_party/binding/napi/napi_base_wrap.h"

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
namespace test {

namespace {
const uint64_t kTestContextClassID = reinterpret_cast<uint64_t>(&kTestContextClassID);
const uint64_t kTestContextConstructorID = reinterpret_cast<uint64_t>(&kTestContextConstructorID);

using Wrapped = binding::NapiBaseWrapped<NapiTestContext>;
typedef Value (NapiTestContext::*InstanceCallback)(const CallbackInfo& info);
typedef void (NapiTestContext::*InstanceSetterCallback)(const CallbackInfo& info, const Value& value);

__attribute__((unused))
void AddAttribute(std::vector<Wrapped::PropertyDescriptor>& props,
                  const char* name,
                  InstanceCallback getter,
                  InstanceSetterCallback setter) {
  props.push_back(
      Wrapped::InstanceAccessor(name, getter, setter, napi_default_jsproperty));
}

__attribute__((unused))
void AddInstanceMethod(std::vector<Wrapped::PropertyDescriptor>& props,
                       const char* name,
                       InstanceCallback method) {
  props.push_back(
      Wrapped::InstanceMethod(name, method, napi_default_jsproperty));
}
}  // namespace

NapiTestContext::NapiTestContext(const CallbackInfo& info, bool skip_init_as_base)
    : NapiBridge(info) {
  set_type_id((void*)&kTestContextClassID);

  // If this is a base class or created by native, skip initialization since
  // impl side needs to have control over the construction of the impl object.
  if (skip_init_as_base || (info.Length() == 1 && info[0].IsExternal())) {
    return;
  }
  ExceptionMessage::IllegalConstructor(info.Env(), InterfaceName());
  return;
}

TestContext* NapiTestContext::ToImplUnsafe() {
  return impl_.get();
}

// static
Object NapiTestContext::Wrap(std::unique_ptr<TestContext> impl, Napi::Env env) {
  DCHECK(impl);
  auto obj = Constructor(env).New({Napi::External::New(env, nullptr, nullptr, nullptr)});
  ObjectWrap<NapiTestContext>::Unwrap(obj)->Init(std::move(impl));
  return obj;
}

// static
bool NapiTestContext::IsInstance(Napi::ScriptWrappable* wrappable) {
  if (!wrappable) {
    return false;
  }
  if (static_cast<NapiTestContext*>(wrappable)->type_id() == &kTestContextClassID) {
    return true;
  }
  return false;
}

void NapiTestContext::Init(std::unique_ptr<TestContext> impl) {
  DCHECK(impl);
  DCHECK(!impl_);

  impl_ = std::move(impl);
  // We only associate and call OnWrapped() once, when we init the root base.
  impl_->AssociateWithWrapper(this);
}

Value NapiTestContext::TestPlusOneMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "TestPlusOne", "1");
    return Value();
  }

  auto arg0_num = NativeValueTraits<IDLNumber>::NativeValue(info, 0);

  auto&& result = impl_->TestPlusOne(arg0_num);
  return Number::New(info.Env(), result);
}

// static
Napi::Class* NapiTestContext::Class(Napi::Env env) {
  auto* clazz = env.GetInstanceData<Napi::Class>(kTestContextClassID);
  if (clazz) {
    return clazz;
  }

  std::vector<Wrapped::PropertyDescriptor> props;

  // Attributes

  // Methods
  AddInstanceMethod(props, "testPlusOne", &NapiTestContext::TestPlusOneMethod);

  // Cache the class
  clazz = new Napi::Class(Wrapped::DefineClass(env, "TestContext", props));
  env.SetInstanceData<Napi::Class>(kTestContextClassID, clazz);
  return clazz;
}

// static
Function NapiTestContext::Constructor(Napi::Env env) {
  FunctionReference* ref = env.GetInstanceData<FunctionReference>(kTestContextConstructorID);
  if (ref) {
    return ref->Value();
  }

  // Cache the constructor for future use
  ref = new FunctionReference();
  ref->Reset(Class(env)->Get(env), 1);
  env.SetInstanceData<FunctionReference>(kTestContextConstructorID, ref);
  return ref->Value();
}

// static
void NapiTestContext::Install(Napi::Env env, Object& target) {
  if (target.Has("TestContext")) {
    return;
  }
  target.Set("TestContext", Constructor(env));
}

}  // namespace test
}  // namespace lynx
