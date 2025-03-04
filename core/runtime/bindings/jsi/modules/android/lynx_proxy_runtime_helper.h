// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_PROXY_RUNTIME_HELPER_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_PROXY_RUNTIME_HELPER_H_

#include <memory>

#include "core/runtime/piper/js/lynx_runtime_helper.h"

namespace lynx {
namespace runtime {

class LynxProxyRuntimeHelper : public LynxRuntimeHelper {
 public:
  static LynxProxyRuntimeHelper& Instance();
  void InitWithRuntimeHelper(LynxRuntimeHelper* ptr);
  bool IsValid() { return helper_valid_; }
  std::unique_ptr<piper::Runtime> MakeRuntime();
  std::shared_ptr<profile::V8RuntimeProfilerWrapper> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext>);

 private:
  std::atomic<bool> helper_valid_ = false;
  LynxRuntimeHelper* helper_ = nullptr;
};
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_ANDROID_LYNX_PROXY_RUNTIME_HELPER_H_
