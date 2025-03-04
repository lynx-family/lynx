// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INSPECTOR_CLIENT_PROVIDER_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INSPECTOR_CLIENT_PROVIDER_H_

#include <memory>

#include "devtool/js_inspect/quickjs/quickjs_inspector_client_impl.h"

namespace lynx {
namespace devtool {

// A thread-local singleton which creates QJSInspectorClientImpl instance.
// All functions declared here must be called on the JS thread.
class QuickjsInspectorClientProvider {
 public:
  static QuickjsInspectorClientProvider* GetInstance();
  std::shared_ptr<QJSInspectorClientImpl> GetInspectorClient();

  QuickjsInspectorClientProvider(const QuickjsInspectorClientProvider&) =
      delete;
  QuickjsInspectorClientProvider& operator=(
      const QuickjsInspectorClientProvider&) = delete;
  QuickjsInspectorClientProvider(QuickjsInspectorClientProvider&&) = delete;
  QuickjsInspectorClientProvider& operator=(QuickjsInspectorClientProvider&&) =
      delete;

 private:
  QuickjsInspectorClientProvider() = default;

  std::shared_ptr<QJSInspectorClientImpl> qjs_client_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INSPECTOR_CLIENT_PROVIDER_H_
