// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/android/lynx_proxy_runtime_helper.h"

#include <mutex>

#include "base/include/base_export.h"
#include "base/include/log/logging.h"

namespace lynx {
namespace runtime {

LynxProxyRuntimeHelper& LynxProxyRuntimeHelper::Instance() {
  static LynxProxyRuntimeHelper instance;
  return instance;
}

void LynxProxyRuntimeHelper::InitWithRuntimeHelper(LynxRuntimeHelper* ptr) {
  if (helper_valid_) {
    LOGW("InitWithRuntimeHelper has already been initialized.");
    return;
  }

  if (ptr == nullptr) {
    LOGW("InitWithRuntimeHelper param error: null.");
    return;
  }

  helper_ = ptr;
  helper_valid_ = true;
  LOGI("InitWithRuntimeHelper success.");
}

std::unique_ptr<piper::Runtime> LynxProxyRuntimeHelper::MakeRuntime() {
  if (!helper_valid_) {
    return nullptr;
  }
  return helper_->MakeRuntime();
}

BASE_EXPORT void RegisterExternalRuntimeHelper(LynxRuntimeHelper* ptr) {
  LynxProxyRuntimeHelper::Instance().InitWithRuntimeHelper(ptr);
}

BASE_EXPORT bool IsExternalRuntimeHelperValid() {
  return LynxProxyRuntimeHelper::Instance().IsValid();
}

std::shared_ptr<profile::V8RuntimeProfilerWrapper>
LynxProxyRuntimeHelper::MakeRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context) {
  if (!helper_valid_) {
    return nullptr;
  }
  return helper_->MakeRuntimeProfiler(js_context);
}

}  // namespace runtime
}  // namespace lynx
