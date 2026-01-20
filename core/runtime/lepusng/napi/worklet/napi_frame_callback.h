// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_callback_function.h.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#ifndef CORE_RUNTIME_LEPUSNG_NAPI_WORKLET_NAPI_FRAME_CALLBACK_H_
#define CORE_RUNTIME_LEPUSNG_NAPI_WORKLET_NAPI_FRAME_CALLBACK_H_

#include <utility>
#include <memory>

#include "third_party/binding/napi/callback_helper.h"
#include "third_party/binding/napi/napi_bridge.h"
#include "third_party/binding/napi/native_value_traits.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace worklet {

using binding::HolderStorage;
using binding::InstanceGuard;

extern const uint64_t kNapiFrameCallbackClassID;

class NapiFrameCallback {
 public:
  NapiFrameCallback(Napi::Function callback);

  NapiFrameCallback(const NapiFrameCallback& cb) = delete;

  void Invoke(int64_t arg0) {
    bool valid;
    Napi::Env env = Env(&valid);
    if (!valid) {
      return;
    }

    Napi::ContextScope cs(env);
    Napi::HandleScope hs(env);

    HolderStorage *storage = reinterpret_cast<HolderStorage*>(env.GetInstanceData(kNapiFrameCallbackClassID));
    DCHECK(storage);

    auto cb = storage->PopHolder(reinterpret_cast<uintptr_t>(this));

    Napi::Value arg0_status;
    arg0_status = Napi::Number::New(env, arg0);

    // The JS callback object is stolen after the call.
    binding::CallbackHelper::Invoke(std::move(cb), result_, exception_handler_, { arg0_status });
  }

  Napi::Value GetResult() { return result_; }
  Napi::Env Env(bool *valid);

  void SetExceptionHandler(std::function<void(Napi::Env)> handler) {
    exception_handler_ = std::move(handler);
  }

 private:
  std::weak_ptr<InstanceGuard> storage_guard_;
  Napi::Value result_;
  std::function<void(Napi::Env)> exception_handler_;
};

}  // namespace worklet
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUSNG_NAPI_WORKLET_NAPI_FRAME_CALLBACK_H_
