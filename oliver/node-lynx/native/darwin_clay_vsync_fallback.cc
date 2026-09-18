// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/vsync_waiter_service.h"
#include "clay/shell/common/vsync_waiter_fallback.h"

namespace clay {

std::shared_ptr<VsyncWaiterService> VsyncWaiterService::Create() {
  // Node embedders drive tasks without an AppKit display-link run loop.
  return CreateFallbackVsyncWaiterService();
}

}  // namespace clay
