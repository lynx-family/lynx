// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/mtl_fence_sync.h"

namespace clay {

MTLCompleteFenceSync::MTLCompleteFenceSync(id<MTLCommandBuffer> buffer) : buffer_(buffer) {}

bool MTLCompleteFenceSync::ClientWait() {
  if ([buffer_ status] != MTLCommandBufferStatusCompleted) {
    [buffer_ waitUntilCompleted];
  }
  return true;
}

FenceSync::Type MTLCompleteFenceSync::GetType() const { return Type::kClientWaitOnly; }

MTLCompleteFenceSync::~MTLCompleteFenceSync() = default;

MTLSharedEventFenceSync::MTLSharedEventFenceSync(id<MTLCommandBuffer> buffer,
                                                 id<MTLSharedEvent> event, uint64_t value)
    : buffer_(buffer), event_(event), value_(value) {}

MTLSharedEventFenceSync::~MTLSharedEventFenceSync() = default;

bool MTLSharedEventFenceSync::ClientWait() {
  if ([buffer_ status] != MTLCommandBufferStatusCompleted) {
    [buffer_ waitUntilCompleted];
  }
  return true;
}

FenceSync::Type MTLSharedEventFenceSync::GetType() const { return Type::kMetalEvent; }

}  // namespace clay
