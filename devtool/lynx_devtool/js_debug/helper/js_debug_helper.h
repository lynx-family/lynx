// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_HELPER_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_HELPER_H_

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "core/inspector/lepus_inspector_manager.h"
#include "core/inspector/runtime_inspector_manager.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/profile/runtime_profiler.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy.h"

namespace lynx {
namespace devtool {

class JSDebugHelper {
 public:
  BASE_EXPORT static JSDebugHelper* GetInstance();

  BASE_EXPORT void SetJSDebugProxy(std::unique_ptr<JSDebugProxy> proxy) {
    proxy_ = std::move(proxy);
  }

  bool IsHelperAvailable() const { return proxy_ != nullptr; }

  std::unique_ptr<piper::RuntimeInspectorManager> CreateRuntimeInspectorManager(
      const std::string& vm_type);
  std::unique_ptr<lepus::LepusInspectorManager> CreateLepusInspectorManager();

  void RegisterNapiRuntimeProxy();
  std::shared_ptr<piper::Runtime> MakeRuntime(const std::string& vm_type);
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context,
      const std::string& vm_type);
#endif

  JSDebugHelper(const JSDebugHelper&) = delete;
  JSDebugHelper& operator=(const JSDebugHelper&) = delete;
  JSDebugHelper(JSDebugHelper&&) = delete;
  JSDebugHelper& operator=(JSDebugHelper&&) = delete;

 private:
  JSDebugHelper() = default;

  std::unique_ptr<JSDebugProxy> proxy_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_HELPER_H_
