// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_V8_V8_INSPECTOR_CLIENT_PROVIDER_H_
#define DEVTOOL_JS_INSPECT_V8_V8_INSPECTOR_CLIENT_PROVIDER_H_

#include <memory>

#include "devtool/js_inspect/v8/v8_inspector_client_impl.h"

namespace lynx {
namespace devtool {

// A thread-local singleton which creates V8InspectorClientImpl instance.
// All functions declared here must be called on the JS thread.
class V8InspectorClientProvider {
 public:
  static V8InspectorClientProvider* GetInstance();
  std::shared_ptr<V8InspectorClientImpl> GetInspectorClient();

  V8InspectorClientProvider(const V8InspectorClientProvider&) = delete;
  V8InspectorClientProvider& operator=(const V8InspectorClientProvider&) =
      delete;
  V8InspectorClientProvider(V8InspectorClientProvider&&) = delete;
  V8InspectorClientProvider& operator=(V8InspectorClientProvider&&) = delete;

 private:
  V8InspectorClientProvider() = default;

  std::shared_ptr<V8InspectorClientImpl> v8_client_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_V8_V8_INSPECTOR_CLIENT_PROVIDER_H_
