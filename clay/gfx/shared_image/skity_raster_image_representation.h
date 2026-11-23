// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SKITY_RASTER_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SKITY_RASTER_IMAGE_REPRESENTATION_H_

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

// CPU copy for render contexts that cannot import a producer's native texture,
// including a macOS SwiftShader context reading an IOSurface. The copied image
// retains its source encoding so normal Skity composition performs conversion.
class SkityRasterImageRepresentation final : public SkityImageRepresentation {
 public:
  explicit SkityRasterImageRepresentation(
      fml::RefPtr<SharedImageBacking> backing);
  std::shared_ptr<SkityImage> GetSkityImage() override;
  bool EndRead() override { return true; }
  void ConsumeFence(std::unique_ptr<FenceSync> fence) override;
  std::unique_ptr<FenceSync> ProduceFence() override;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SKITY_RASTER_IMAGE_REPRESENTATION_H_
