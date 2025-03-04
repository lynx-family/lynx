// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INSPECTOR_CLIENT_PROVIDER_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INSPECTOR_CLIENT_PROVIDER_H_

#include <memory>

#include "devtool/js_inspect/lepus/lepus_inspector_client_impl.h"

namespace lynx {
namespace devtool {

class LepusInspectorClientProvider {
 public:
  static std::shared_ptr<LepusInspectorClientImpl> GetInspectorClient();
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INSPECTOR_CLIENT_PROVIDER_H_
