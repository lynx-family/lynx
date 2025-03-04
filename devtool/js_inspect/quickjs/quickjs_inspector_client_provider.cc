// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_inspector_client_provider.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace devtool {

QuickjsInspectorClientProvider* QuickjsInspectorClientProvider::GetInstance() {
  static thread_local QuickjsInspectorClientProvider instance_;
  return &instance_;
}

std::shared_ptr<QJSInspectorClientImpl>
QuickjsInspectorClientProvider::GetInspectorClient() {
  if (qjs_client_ == nullptr) {
    qjs_client_ = std::make_shared<QJSInspectorClientImpl>();
    LOGI("js debug: create QJSInspectorClientImpl " << qjs_client_);
  }
  return qjs_client_;
}

}  // namespace devtool
}  // namespace lynx
