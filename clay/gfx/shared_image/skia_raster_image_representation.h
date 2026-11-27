// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SKIA_RASTER_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SKIA_RASTER_IMAGE_REPRESENTATION_H_

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

// An owned CPU snapshot, with the same bottom-left source origin as desktop
// GPU representations. The image remains valid after returning the buffer.
class SkiaRasterImageRepresentation final : public SkiaImageRepresentation {
 public:
  explicit SkiaRasterImageRepresentation(
      fml::RefPtr<SharedImageBacking> backing);
  sk_sp<SkImage> GetSkImage() override;
  bool EndRead() override { return !HasFailedFence(); }
  void ConsumeFence(std::unique_ptr<FenceSync> fence) override;
  std::unique_ptr<FenceSync> ProduceFence() override;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SKIA_RASTER_IMAGE_REPRESENTATION_H_
