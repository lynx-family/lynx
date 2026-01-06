// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_PROXY_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_PROXY_H_

#include <memory>

namespace lynx {

namespace runtime {
namespace profile {
class RuntimeProfiler;
}  // namespace profile
}  // namespace runtime

namespace piper {
class Runtime;
class JSIContext;
class RuntimeInspectorManager;
}  // namespace piper

namespace lepus {
class LepusInspectorManager;
}  // namespace lepus

namespace devtool {

class JSDebugProxy {
 public:
  JSDebugProxy() = default;
  virtual ~JSDebugProxy() = default;

  virtual std::unique_ptr<piper::RuntimeInspectorManager>
  CreateRuntimeInspectorManager() {
    return nullptr;
  }
  virtual std::unique_ptr<lepus::LepusInspectorManager>
  CreateLepusInspectorManager() {
    return nullptr;
  }

  virtual void RegisterNapiRuntimeProxy() {}
  virtual std::shared_ptr<piper::Runtime> MakeRuntime() { return nullptr; }
#if ENABLE_TRACE_PERFETTO
  virtual std::shared_ptr<runtime::profile::RuntimeProfiler>
  MakeRuntimeProfiler(std::shared_ptr<piper::JSIContext> js_context) {
    return nullptr;
  }
#endif
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_HELPER_JS_DEBUG_PROXY_H_
