// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_PROXY_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_PROXY_IMPL_H_

#include <memory>
#include <string>

#include "core/inspector/runtime_inspector_manager.h"
#include "core/runtime/jsi/jsi.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy.h"

namespace lynx {
namespace devtool {

class JSDebugProxyImpl : public JSDebugProxy {
 public:
  JSDebugProxyImpl() = default;
  ~JSDebugProxyImpl() override = default;

  std::unique_ptr<piper::RuntimeInspectorManager> CreateRuntimeInspectorManager(
      const std::string& vm_type) override;
  std::unique_ptr<lepus::LepusInspectorManager> CreateLepusInspectorManager()
      override;

  void RegisterNapiRuntimeProxy() override;
  std::shared_ptr<piper::Runtime> MakeRuntime(
      const std::string& vm_type) override;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context,
      const std::string& vm_type) override;
#endif
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_PROXY_IMPL_H_
