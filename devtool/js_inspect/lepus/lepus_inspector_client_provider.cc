// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_inspector_client_provider.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace devtool {

std::shared_ptr<LepusInspectorClientImpl>
LepusInspectorClientProvider::GetInspectorClient() {
  auto client = std::make_shared<LepusInspectorClientImpl>();
  LOGI("lepus debug: create LepusInspectorClientImpl " << client);
  return client;
}

}  // namespace devtool
}  // namespace lynx
