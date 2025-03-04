// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "jsbridge/bindings/gen_test/napi_async_object.h"

#include "jsbridge/bindings/gen_test/napi_gen_test_command_buffer.h"
#include "third_party/binding/napi/napi_base_wrap.h"

using Napi::CallbackInfo;
using Napi::Number;
using Napi::Object;

namespace lynx {
namespace gen_test {

namespace {
const uint64_t kAsyncObjectClassID =
    reinterpret_cast<uint64_t>(&kAsyncObjectClassID);

using Wrapped = binding::NapiBaseWrapped<NapiAsyncObject>;
}  // namespace

NapiAsyncObject::NapiAsyncObject(const CallbackInfo& info) : NapiBridge(info) {
  set_type_id((void*)&kAsyncObjectClassID);

  id_ = info[0].As<Number>().Uint32Value();
  NapiGenTestCommandBuffer::RegisterAsyncObject(this);
}

NapiAsyncObject::~NapiAsyncObject() {
  if (impl_) {
    impl_->Dispose();
  }
  NapiGenTestCommandBuffer::UnregisterAsyncObject(this);
}

// static
bool NapiAsyncObject::IsInstance(Napi::ScriptWrappable* wrappable) {
  if (!wrappable) {
    return false;
  }
  if (static_cast<NapiAsyncObject*>(wrappable)->type_id() ==
      &kAsyncObjectClassID) {
    return true;
  }
  return false;
}

void NapiAsyncObject::Init(std::unique_ptr<ImplBase> impl) {
  DCHECK(impl);
  DCHECK(!impl_);

  impl_ = std::move(impl);
  // We only associate and call OnWrapped() once, when we init the root base.
  impl_->AssociateWithWrapper(this);
}

// static
void NapiAsyncObject::Install(Napi::Env env, Object& target, const char* name) {
  if (target.Has(name).FromMaybe(false)) {
    return;
  }
  std::vector<Wrapped::PropertyDescriptor> props;

  Napi::Function func = Wrapped::DefineClass(env, name, props).Get(env);
  Napi::FunctionReference* ref = new Napi::FunctionReference();
  ref->Reset(func, 1);
  env.SetInstanceData<Napi::FunctionReference>(reinterpret_cast<uint64_t>(name),
                                               ref);

  target.Set(name, ref->Value());
}

}  // namespace gen_test
}  // namespace lynx
