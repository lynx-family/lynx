// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "devtool/embedder/core/global_devtool_platform_embedder.h"

namespace lynx {
namespace devtool {

void GlobalDevtoolPlatformEmbedder::GetAllMemoryUsage(
    int64_t timeout_ms,
    base::MoveOnlyClosure<void, const std::string&, const std::string&>
        callback) {
  (void)timeout_ms;
  // The generic embedder has no host-specific memory bridge. Keep the CDP
  // plumbing honest by returning an explicit error until embedders provide
  // their own memory counters through this static hook.
  constexpr char kBridgeUnavailable[] =
      "Memory.getAllMemoryUsage is not available in this Lynx runtime. "
      "Upgrade the Lynx runtime and DevTool platform integration to a version "
      "with the global memory bridge.";
  if (callback) {
    std::move(callback)("{}", kBridgeUnavailable);
  }
}

}  // namespace devtool
}  // namespace lynx
