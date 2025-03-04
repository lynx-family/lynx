// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_callback_function.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "core/runtime/bindings/napi/worklet/napi_frame_callback.h"

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

namespace lynx {
namespace worklet {
namespace {
  const uint64_t kNapiFrameCallbackClassID = reinterpret_cast<uint64_t>(&kNapiFrameCallbackClassID);
}

void NapiFrameCallback::Invoke(int64_t arg0) {
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

NapiFrameCallback::NapiFrameCallback(Napi::Function callback) {
  Napi::Env env = callback.Env();
  HolderStorage *storage = reinterpret_cast<HolderStorage*>(env.GetInstanceData(kNapiFrameCallbackClassID));
  if (storage == nullptr) {
    storage = new HolderStorage();
    env.SetInstanceData(kNapiFrameCallbackClassID, storage, [](napi_env env, void* finalize_data,
                                                                   void* finalize_hint) { delete reinterpret_cast<HolderStorage*>(finalize_data); }, nullptr);
  }

  storage->PushHolder(reinterpret_cast<uintptr_t>(this), Napi::Persistent(callback));

  storage_guard_ = storage->instance_guard();
}

Napi::Env NapiFrameCallback::Env(bool *valid) {
  if (valid != nullptr) {
    *valid = false;
  }

  auto strong_guard = storage_guard_.lock();
  if (!strong_guard) {
    // if valid is nullptr, it must be valid.
    DCHECK(valid);
    return Napi::Env(nullptr);
  }

  auto storage = strong_guard->Get();
  auto &cb = storage->PeekHolder(reinterpret_cast<uintptr_t>(this));
  if (cb.IsEmpty()) {
    // if valid is nullptr, it must be valid.
    DCHECK(valid);
    return Napi::Env(nullptr);
  }

  if (valid != nullptr) {
    *valid = true;
  }
  return cb.Env();
}

}  // namespace worklet
}  // namespace lynx
