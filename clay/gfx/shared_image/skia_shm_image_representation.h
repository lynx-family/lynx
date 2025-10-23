// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SKIA_SHM_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SKIA_SHM_IMAGE_REPRESENTATION_H_

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class SkiaShmImageRepresentation final : public SkiaImageRepresentation {
 public:
  explicit SkiaShmImageRepresentation(
      fml::RefPtr<SharedImageRepresentation> representation);
  ~SkiaShmImageRepresentation() override;

  sk_sp<SkImage> GetSkImage() override;
  bool EndRead() override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

 private:
  fml::RefPtr<SharedImageRepresentation> representation_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SKIA_SHM_IMAGE_REPRESENTATION_H_
