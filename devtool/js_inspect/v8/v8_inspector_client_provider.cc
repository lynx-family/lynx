// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/v8/v8_inspector_client_provider.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace devtool {

V8InspectorClientProvider* V8InspectorClientProvider::GetInstance() {
  static thread_local V8InspectorClientProvider instance_;
  return &instance_;
}

std::shared_ptr<V8InspectorClientImpl>
V8InspectorClientProvider::GetInspectorClient() {
  if (v8_client_ == nullptr) {
    v8_client_ = std::make_shared<V8InspectorClientImpl>();
    LOGI("js debug: create V8InspectorClientImpl " << v8_client_);
  }
  return v8_client_;
}

}  // namespace devtool
}  // namespace lynx
