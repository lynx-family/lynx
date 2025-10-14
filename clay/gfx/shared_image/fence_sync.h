// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_FENCE_SYNC_H_
#define CLAY_GFX_SHARED_IMAGE_FENCE_SYNC_H_

#include "base/include/fml/macros.h"

namespace clay {

class FenceSync {
 public:
  enum class Type {
    kCGL,
    kEAGL,
    kEGL,
    kANGLE,
    kMetalEvent,
    kClientWaitOnly,
    kVulkan,
  };

  FenceSync() = default;

  virtual ~FenceSync() = default;

  virtual bool ClientWait() = 0;

  virtual Type GetType() const = 0;

  BASE_DISALLOW_COPY_AND_ASSIGN(FenceSync);
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_FENCE_SYNC_H_
