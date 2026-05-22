// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_MTL_FENCE_SYNC_H_
#define CLAY_GFX_SHARED_IMAGE_MTL_FENCE_SYNC_H_

#import <Metal/Metal.h>

#include <future>
#include <string>

#include "clay/fml/platform/darwin/scoped_nsobject.h"
#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

class MTLCompleteFenceSync final : public FenceSync {
 public:
  explicit MTLCompleteFenceSync(id<MTLCommandBuffer> buffer);

  bool ClientWait() override;

  Type GetType() const override;

  ~MTLCompleteFenceSync() override;

 private:
  fml::scoped_nsprotocol<id<MTLCommandBuffer>> buffer_;
};

class API_AVAILABLE(macos(10.14), ios(12.0),
                    tvos(12.0)) MTLSharedEventFenceSync final
    : public FenceSync {
 public:
  MTLSharedEventFenceSync(id<MTLCommandBuffer> buffer, id<MTLSharedEvent> event,
                          uint64_t value);
  ~MTLSharedEventFenceSync() override;

  bool ClientWait() override;

  Type GetType() const override;

  id<MTLSharedEvent> Event() const { return event_; }

  uint64_t Value() const { return value_; }

 private:
  fml::scoped_nsprotocol<id<MTLCommandBuffer>> buffer_;
  fml::scoped_nsprotocol<id<MTLSharedEvent>> event_;
  uint64_t value_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_MTL_FENCE_SYNC_H_
