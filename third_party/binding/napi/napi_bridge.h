// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_BASE_H_
#define BINDING_NAPI_BASE_H_

#include "third_party/binding/common/base.h"
#include "third_party/binding/common/env.h"
#include "third_party/binding/napi/napi_value.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

class NapiBridge : public Napi::ScriptWrappable, public BridgeBase {
 public:
  Object GetBaseObject() override;
  Napi::Object NapiObject() override;
  ObjectRef GetStrongRef() override;
  Env GetEnv() override;
  Napi::Env NapiEnv() override { return env_; }
  Napi::Env Env() { return env_; }
  bool IsNapi() override { return true; }

  typedef Napi::Value (NapiBridge::*InstanceCallbackPtr)(
      const Napi::CallbackInfo& info);
  typedef Napi::Value (NapiBridge::*GetterCallbackPtr)(
      const Napi::CallbackInfo& info);
  typedef void (NapiBridge::*SetterCallbackPtr)(const Napi::CallbackInfo& info,
                                                const Napi::Value& value);
  typedef Napi::Value (*StaticMethodCallback)(const Napi::CallbackInfo& info);
  typedef void (*StaticSetterCallback)(const Napi::CallbackInfo& info,
                                       const Napi::Value& value);

 protected:
  explicit NapiBridge(const Napi::CallbackInfo& info);
  ~NapiBridge() override;

 private:
  Napi::ObjectReference weak_ref_;
  Napi::Env env_;
};

class NapiEnvImpl : public EnvImpl {
 public:
  static NapiEnvImpl* From(Napi::Env env);

  bool IsNapi() const override { return true; }

  void* GetInstanceData(uint64_t key) override;
  void SetInstanceData(uint64_t key, void* data, EnvDataFinalizer cb,
                       void* hint) override;

 private:
  friend Napi::Env ToNAPI(Env env);
  explicit NapiEnvImpl(Napi::Env env) : env_(env) {}

  Napi::Env env_;
};

Env FromNAPI(Napi::Env env);
Napi::Env ToNAPI(Env env);

}  // namespace binding
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // BINDING_NAPI_BASE_H_
