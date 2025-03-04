// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/napi_bridge.h"

#include "third_party/binding/common/object.h"
#include "third_party/binding/common/object_ref.h"
#include "third_party/binding/napi/napi_object.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

NapiBridge::NapiBridge(const Napi::CallbackInfo& info) : env_(info.Env()) {
  weak_ref_.Reset(info.This().ToObject());
}

NapiBridge::~NapiBridge() = default;

Object NapiBridge::GetBaseObject() { return FromNAPI(weak_ref_.Value()); }

Napi::Object NapiBridge::NapiObject() { return weak_ref_.Value(); }

ObjectRef NapiBridge::GetStrongRef() { return GetBaseObject().AdoptRef(); }

Env NapiBridge::GetEnv() { return FromNAPI(env_); }

namespace {
const uint64_t kNapiEnvImplDataKey =
    reinterpret_cast<uint64_t>(&kNapiEnvImplDataKey);
}  // namespace

// static
NapiEnvImpl* NapiEnvImpl::From(Napi::Env env) {
  auto* impl = env.GetInstanceData<NapiEnvImpl>(kNapiEnvImplDataKey);
  if (!impl) {
    impl = new NapiEnvImpl(env);
    env.SetInstanceData(kNapiEnvImplDataKey, impl);
  }
  return impl;
}

void* NapiEnvImpl::GetInstanceData(uint64_t key) {
  return env_.GetInstanceData(key);
}

void NapiEnvImpl::SetInstanceData(uint64_t key, void* data, EnvDataFinalizer cb,
                                  void* hint) {
  struct AdaptorData {
    EnvDataFinalizer cb;
    void* hint;
  };
  AdaptorData* adaptor_data = new AdaptorData{cb, hint};
  env_.SetInstanceData(
      key, data,
      [](napi_env env, void* data, void* hint) {
        AdaptorData* adaptor_data = static_cast<AdaptorData*>(hint);
        adaptor_data->cb(FromNAPI(env), data, adaptor_data->hint);
        delete adaptor_data;
      },
      adaptor_data);
}

Env FromNAPI(Napi::Env env) { return Env(NapiEnvImpl::From(env)); }

Napi::Env ToNAPI(Env env) {
  if (!env.IsNapi()) {
    return nullptr;
  }
  return env.GetInstanceData<NapiEnvImpl>(kNapiEnvImplDataKey)->env_;
}

}  // namespace binding
}  // namespace lynx
