// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_V8_PROXY_JS_DEBUG_PROXY_V8_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_V8_PROXY_JS_DEBUG_PROXY_V8_H_

#include <memory>

#include "core/runtime/js/jsi/jsi.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy.h"

namespace lynx {
namespace devtool {

class JSDebugProxyV8 : public JSDebugProxy {
 public:
  JSDebugProxyV8() = default;
  ~JSDebugProxyV8() override = default;

  std::unique_ptr<runtime::js::RuntimeInspectorManager>
  CreateRuntimeInspectorManager() override;

  void RegisterNapiRuntimeProxy() override;
  std::shared_ptr<runtime::js::Runtime> MakeRuntime() override;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<runtime::profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<runtime::js::JSIContext> js_context) override;
#endif
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_V8_PROXY_JS_DEBUG_PROXY_V8_H_
