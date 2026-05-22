// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SKITY_MTL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SKITY_MTL_IMAGE_REPRESENTATION_H_

#import <Metal/Metal.h>

#include <memory>

#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class IOSurfaceImageBacking;
class CVPixelBufferImageBacking;
class MTLImageRepresentation;

class API_AVAILABLE(macos(10.6), ios(11.0),
                    tvos(11.0)) SkityMTLImageRepresentation final
    : public SkityImageRepresentation {
 public:
  SkityMTLImageRepresentation(GrContext* skity_context,
                              fml::RefPtr<IOSurfaceImageBacking> backing);
  SkityMTLImageRepresentation(GrContext* skity_context,
                              fml::RefPtr<CVPixelBufferImageBacking> backing);
  ~SkityMTLImageRepresentation() override;

  std::shared_ptr<SkityImage> GetSkityImage() override;
  bool EndRead() override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  fml::RefPtr<RepresentationStorageManager> GetTextureManager() const override;
  void SetTextureManager(
      fml::RefPtr<RepresentationStorageManager> storage_manager) override;

 private:
  GrContext* skity_context_;
  fml::RefPtr<MTLImageRepresentation> mtl_image_representation_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SKITY_MTL_IMAGE_REPRESENTATION_H_
