// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/logbox/lynx_logbox_bridge.h"

namespace lynx {
namespace embedder {

std::unique_ptr<LynxLogBoxBridge> LynxLogBoxBridge::Create(
    LogBoxResourceProvider* provider) {
  return nullptr;
}

}  // namespace embedder
}  // namespace lynx
