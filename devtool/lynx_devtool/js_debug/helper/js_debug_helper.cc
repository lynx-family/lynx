// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"

namespace lynx {
namespace devtool {

JSDebugHelper* JSDebugHelper::GetInstance() {
  static JSDebugHelper instance_;
  return &instance_;
}

std::unique_ptr<piper::RuntimeInspectorManager>
JSDebugHelper::CreateRuntimeInspectorManager(const std::string& vm_type) {
  if (proxy_ == nullptr) {
    return nullptr;
  }
  return proxy_->CreateRuntimeInspectorManager(vm_type);
}

std::unique_ptr<lepus::LepusInspectorManager>
JSDebugHelper::CreateLepusInspectorManager() {
  if (proxy_ == nullptr) {
    return nullptr;
  }
  return proxy_->CreateLepusInspectorManager();
}

void JSDebugHelper::RegisterNapiRuntimeProxy() {
  if (proxy_ == nullptr) {
    return;
  }
  proxy_->RegisterNapiRuntimeProxy();
}

std::shared_ptr<piper::Runtime> JSDebugHelper::MakeRuntime(
    const std::string& vm_type) {
  if (proxy_ == nullptr) {
    return nullptr;
  }
  return proxy_->MakeRuntime(vm_type);
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<runtime::profile::RuntimeProfiler>
JSDebugHelper::MakeRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context, const std::string& vm_type) {
  if (proxy_ == nullptr) {
    return nullptr;
  }
  return proxy_->MakeRuntimeProfiler(js_context, vm_type);
}
#endif

}  // namespace devtool
}  // namespace lynx
