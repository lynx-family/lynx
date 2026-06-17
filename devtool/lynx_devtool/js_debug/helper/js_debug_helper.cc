// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"

#include "devtool/js_inspect/inspector_const.h"

namespace lynx {
namespace devtool {

JSDebugHelper* JSDebugHelper::GetInstance() {
  static JSDebugHelper instance_;
  return &instance_;
}

std::unique_ptr<runtime::js::RuntimeInspectorManager>
JSDebugHelper::CreateRuntimeInspectorManager(const std::string& vm_type) {
  if (vm_type == kKeyEngineV8) {
    if (v8_proxy_ != nullptr) {
      return v8_proxy_->CreateRuntimeInspectorManager();
    } else if (quickjs_proxy_ != nullptr) {
      LOGW("js debug: v8_proxy_ is not set, use quickjs_proxy_ instead");
      return quickjs_proxy_->CreateRuntimeInspectorManager();
    }
  } else if (vm_type == kKeyEngineQuickjs) {
    if (quickjs_proxy_ != nullptr) {
      return quickjs_proxy_->CreateRuntimeInspectorManager();
    } else if (v8_proxy_ != nullptr) {
      LOGW("js debug: quickjs_proxy_ is not set, use v8_proxy_ instead");
      return v8_proxy_->CreateRuntimeInspectorManager();
    }
  }
  LOGW("js debug: no proxy_ set for vm_type: " << vm_type);
  return nullptr;
}

std::unique_ptr<lepus::LepusInspectorManager>
JSDebugHelper::CreateLepusInspectorManager() {
  if (lepus_proxy_ == nullptr) {
    LOGW("js debug: lepus_proxy_ is not set");
    return nullptr;
  }
  return lepus_proxy_->CreateLepusInspectorManager();
}

void JSDebugHelper::RegisterNapiRuntimeProxy() {
  if (v8_proxy_ == nullptr) {
    LOGW("js debug: v8_proxy_ is not set");
    return;
  }
  v8_proxy_->RegisterNapiRuntimeProxy();
}

std::shared_ptr<runtime::js::Runtime> JSDebugHelper::MakeRuntime(
    const std::string& vm_type) {
  if (vm_type == kKeyEngineV8) {
    if (v8_proxy_ != nullptr) {
      return v8_proxy_->MakeRuntime();
    } else if (quickjs_proxy_ != nullptr) {
      LOGW("js debug: v8_proxy_ is not set, use quickjs_proxy_ instead");
      return quickjs_proxy_->MakeRuntime();
    }
  } else if (vm_type == kKeyEngineQuickjs) {
    if (quickjs_proxy_ != nullptr) {
      return quickjs_proxy_->MakeRuntime();
    } else if (v8_proxy_ != nullptr) {
      LOGW("js debug: quickjs_proxy_ is not set, use v8_proxy_ instead");
      return v8_proxy_->MakeRuntime();
    }
  }
  LOGW("js debug: no proxy_ set for vm_type: " << vm_type);
  return nullptr;
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<runtime::profile::RuntimeProfiler>
JSDebugHelper::MakeRuntimeProfiler(
    std::shared_ptr<runtime::js::JSIContext> js_context,
    const std::string& vm_type) {
  if (vm_type == kKeyEngineV8) {
    if (v8_proxy_ != nullptr) {
      return v8_proxy_->MakeRuntimeProfiler(js_context);
    } else if (quickjs_proxy_ != nullptr) {
      LOGW("js debug: v8_proxy_ is not set, use quickjs_proxy_ instead");
      return quickjs_proxy_->MakeRuntimeProfiler(js_context);
    }
  } else if (vm_type == kKeyEngineQuickjs) {
    if (quickjs_proxy_ != nullptr) {
      return quickjs_proxy_->MakeRuntimeProfiler(js_context);
    } else if (v8_proxy_ != nullptr) {
      LOGW("js debug: quickjs_proxy_ is not set, use v8_proxy_ instead");
      return v8_proxy_->MakeRuntimeProfiler(js_context);
    }
  }
  LOGW("js debug: no proxy_ set for vm_type: " << vm_type);
  return nullptr;
}
#endif

}  // namespace devtool
}  // namespace lynx
