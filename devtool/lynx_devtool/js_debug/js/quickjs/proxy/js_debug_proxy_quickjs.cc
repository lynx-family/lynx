// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/quickjs/proxy/js_debug_proxy_quickjs.h"

#include "core/runtime/js/jsi/quickjs/quickjs_api.h"
#include "devtool/lynx_devtool/js_debug/js/quickjs/manager/quickjs_inspector_manager_impl.h"

namespace lynx {
namespace devtool {

std::unique_ptr<runtime::js::RuntimeInspectorManager>
JSDebugProxyQuickJS::CreateRuntimeInspectorManager() {
  return std::make_unique<runtime::js::QuickjsInspectorManagerImpl>();
}

std::shared_ptr<runtime::js::Runtime> JSDebugProxyQuickJS::MakeRuntime() {
  LOGI("js debug: make QuickJS runtime");
  return runtime::js::makeQuickJsRuntime();
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<runtime::profile::RuntimeProfiler>
JSDebugProxyQuickJS::MakeRuntimeProfiler(
    std::shared_ptr<runtime::js::JSIContext> js_context) {
  LOGI("js debug: make QuickJS profiler");
  return runtime::js::makeQuickJsRuntimeProfiler(js_context);
}
#endif

}  // namespace devtool
}  // namespace lynx
