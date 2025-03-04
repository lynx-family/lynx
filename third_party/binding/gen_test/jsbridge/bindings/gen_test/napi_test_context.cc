// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_interface.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "jsbridge/bindings/gen_test/napi_test_context.h"

#include <vector>
#include <utility>

#include "base/include/vector.h"

#include "base/include/log/logging.h"
#include "third_party/binding/gen_test/test_context.h"
#include "third_party/binding/napi/exception_message.h"
#include "third_party/binding/napi/napi_base_wrap.h"

#include "jsbridge/bindings/gen_test/napi_gen_test_command_buffer.h"
#include "base/include/fml/make_copyable.h"

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
namespace gen_test {

namespace {
const uint64_t kTestContextClassID = reinterpret_cast<uint64_t>(&kTestContextClassID);
const uint64_t kTestContextConstructorID = reinterpret_cast<uint64_t>(&kTestContextConstructorID);

using Wrapped = binding::NapiBaseWrapped<NapiTestContext>;
typedef Value (NapiTestContext::*InstanceCallback)(const CallbackInfo& info);
typedef void (NapiTestContext::*InstanceSetterCallback)(const CallbackInfo& info, const Value& value);

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

NapiTestContext::~NapiTestContext() {
  // If JS context is being teared down, no need to keep the impl.
  if (static_cast<napi_env>(Env())->rt) {
    NapiGenTestCommandBuffer::OrphanImpl(Env(), std::unique_ptr<ImplBase>(ReleaseImpl()), object_id_);
  }

  NapiGenTestCommandBuffer::UnregisterBufferedObject(this, object_id_);
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

  object_id_ = NapiGenTestCommandBuffer::RegisterBufferedObject(this);
  Napi::PropertyDescriptor js_id =
      Napi::PropertyDescriptor::Value("__id",  Number::New(Env(), object_id_), napi_default);
  NapiObject().DefineProperty(js_id);
  impl_->SetClientOnFrameCallback(
      fml::MakeCopyable([env = Env()]() {
        Napi::HandleScope hs(env);
        NapiGenTestCommandBuffer::FlushCommandBuffer(env, true);
      }));
}

ImplBase* NapiTestContext::ReleaseImpl() {
  impl_->AssociateWithWrapper(nullptr);
  return impl_.release();
}

Value NapiTestContext::VoidFromStringMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "VoidFromString", "1");
    return Value();
  }

  auto arg0_s = NativeValueTraits<IDLString>::NativeValue(info, 0);

  impl_->VoidFromString(std::move(arg0_s));
  return info.Env().Undefined();
}

Value NapiTestContext::StringFromVoidMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  auto&& result = impl_->StringFromVoid();
  return String::New(info.Env(), result);
}

Value NapiTestContext::VoidFromStringArrayMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "VoidFromStringArray", "1");
    return Value();
  }

  auto arg0_sa = NativeValueTraits<IDLSequence<IDLString>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->VoidFromStringArray(std::move(arg0_sa));
  return info.Env().Undefined();
}

Value NapiTestContext::VoidFromTypedArrayMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "VoidFromTypedArray", "1");
    return Value();
  }

  auto arg0_fa = NativeValueTraits<IDLFloat32Array>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->VoidFromTypedArray(arg0_fa);
  return info.Env().Undefined();
}

Value NapiTestContext::VoidFromArrayBufferMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "VoidFromArrayBuffer", "1");
    return Value();
  }

  auto arg0_ab = NativeValueTraits<IDLArrayBuffer>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->VoidFromArrayBuffer(arg0_ab);
  return info.Env().Undefined();
}

Value NapiTestContext::VoidFromArrayBufferViewMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "VoidFromArrayBufferView", "1");
    return Value();
  }

  auto arg0_abv = NativeValueTraits<IDLArrayBufferView>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->VoidFromArrayBufferView(arg0_abv);
  return info.Env().Undefined();
}

Value NapiTestContext::VoidFromNullableArrayBufferViewMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  if (info.Length() < 1) {
    ExceptionMessage::NotEnoughArguments(info.Env(), InterfaceName(), "VoidFromNullableArrayBufferView", "1");
    return Value();
  }

  auto arg0_abv = NativeValueTraits<IDLNullable<IDLArrayBufferView>>::NativeValue(info, 0);
  if (info.Env().IsExceptionPending()) {
    return info.Env().Undefined();
  }

  impl_->VoidFromNullableArrayBufferView(arg0_abv);
  return info.Env().Undefined();
}

Value NapiTestContext::FinishMethod(const CallbackInfo& info) {
  DCHECK(impl_);

  NapiGenTestCommandBuffer::FlushCommandBuffer(info.Env());

  impl_->Finish();
  return info.Env().Undefined();
}

// static
Napi::Class* NapiTestContext::Class(Napi::Env env) {
  auto* clazz = env.GetInstanceData<Napi::Class>(kTestContextClassID);
  if (clazz) {
    return clazz;
  }

  base::InlineVector<Wrapped::PropertyDescriptor, 8> props;

  // Attributes

  // Methods
  AddInstanceMethod(props, "voidFromString_", &NapiTestContext::VoidFromStringMethod);
  AddInstanceMethod(props, "stringFromVoid", &NapiTestContext::StringFromVoidMethod);
  AddInstanceMethod(props, "voidFromStringArray_", &NapiTestContext::VoidFromStringArrayMethod);
  AddInstanceMethod(props, "voidFromTypedArray_", &NapiTestContext::VoidFromTypedArrayMethod);
  AddInstanceMethod(props, "voidFromArrayBuffer_", &NapiTestContext::VoidFromArrayBufferMethod);
  AddInstanceMethod(props, "voidFromArrayBufferView_", &NapiTestContext::VoidFromArrayBufferViewMethod);
  AddInstanceMethod(props, "voidFromNullableArrayBufferView_", &NapiTestContext::VoidFromNullableArrayBufferViewMethod);
  AddInstanceMethod(props, "finish", &NapiTestContext::FinishMethod);

  // Cache the class
  clazz = new Napi::Class(Wrapped::DefineClass(env, "TestContext", props.size(), props.data<const napi_property_descriptor>()));
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
  if (target.Has("TestContext").FromMaybe(false)) {
    return;
  }
  target.Set("TestContext", Constructor(env));
}

}  // namespace gen_test
}  // namespace lynx
