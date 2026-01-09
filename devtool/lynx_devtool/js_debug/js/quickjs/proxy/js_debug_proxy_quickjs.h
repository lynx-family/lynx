// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_QUICKJS_PROXY_JS_DEBUG_PROXY_QUICKJS_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_QUICKJS_PROXY_JS_DEBUG_PROXY_QUICKJS_H_

#include <memory.h>

#include "core/runtime/js/jsi/jsi.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy.h"

namespace lynx {
namespace devtool {

class JSDebugProxyQuickJS : public JSDebugProxy {
 public:
  JSDebugProxyQuickJS() = default;
  ~JSDebugProxyQuickJS() override = default;

  std::unique_ptr<piper::RuntimeInspectorManager>
  CreateRuntimeInspectorManager() override;

  std::shared_ptr<piper::Runtime> MakeRuntime() override;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<runtime::profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context) override;
#endif
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_QUICKJS_PROXY_JS_DEBUG_PROXY_QUICKJS_H_
