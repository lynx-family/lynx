// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_callback_function.h.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#ifndef CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_FUNC_CALLBACK_H_
#define CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_FUNC_CALLBACK_H_

#include <utility>
#include <memory>

#include "third_party/binding/napi/callback_helper.h"
#include "third_party/binding/napi/napi_bridge.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace worklet {

using binding::HolderStorage;
using binding::InstanceGuard;

class NapiFuncCallback {
 public:
  NapiFuncCallback(Napi::Function callback);

  NapiFuncCallback(const NapiFuncCallback& cb) = delete;

  ~NapiFuncCallback();

  void Invoke(Napi::Value arg0);

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

#endif  // CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_FUNC_CALLBACK_H_
