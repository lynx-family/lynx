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
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/profile/runtime_profiler.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy.h"

namespace lynx {
namespace devtool {

class JSDebugHelper {
 public:
  BASE_EXPORT static JSDebugHelper* GetInstance();

  BASE_EXPORT void SetV8Proxy(std::unique_ptr<JSDebugProxy> proxy) {
    v8_proxy_ = std::move(proxy);
  }

  BASE_EXPORT void SetQuickJSProxy(std::unique_ptr<JSDebugProxy> proxy) {
    quickjs_proxy_ = std::move(proxy);
  }

  BASE_EXPORT void SetLepusProxy(std::unique_ptr<JSDebugProxy> proxy) {
    lepus_proxy_ = std::move(proxy);
  }

  bool IsJSDebugAvailable() const {
    return v8_proxy_ != nullptr || quickjs_proxy_ != nullptr;
  }
  bool IsLepusDebugAvailable() const { return lepus_proxy_ != nullptr; }

  std::unique_ptr<piper::RuntimeInspectorManager> CreateRuntimeInspectorManager(
      const std::string& vm_type);
  std::unique_ptr<lepus::LepusInspectorManager> CreateLepusInspectorManager();

  void RegisterNapiRuntimeProxy();
  std::shared_ptr<piper::Runtime> MakeRuntime(const std::string& vm_type);
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<runtime::profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context,
      const std::string& vm_type);
#endif

  JSDebugHelper(const JSDebugHelper&) = delete;
  JSDebugHelper& operator=(const JSDebugHelper&) = delete;
  JSDebugHelper(JSDebugHelper&&) = delete;
  JSDebugHelper& operator=(JSDebugHelper&&) = delete;

 private:
  JSDebugHelper() = default;

  std::unique_ptr<JSDebugProxy> v8_proxy_;
  std::unique_ptr<JSDebugProxy> quickjs_proxy_;
  std::unique_ptr<JSDebugProxy> lepus_proxy_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_HELPER_H_
