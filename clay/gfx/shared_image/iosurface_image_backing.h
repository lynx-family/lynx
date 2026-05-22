// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_IOSURFACE_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_IOSURFACE_IMAGE_BACKING_H_

#import <IOSurface/IOSurfaceRef.h>

#include <memory>
#include <optional>

#include "clay/fml/platform/darwin/cf_utils.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

class SharedImageRepresentation;

class API_AVAILABLE(macos(10.6), ios(11.0), tvos(11.0)) IOSurfaceImageBacking
    : public SharedImageBacking {
 public:
  IOSurfaceImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                        std::optional<GraphicsMemoryHandle> gfx_handle = {});

  BackingType GetType() const override;

  GraphicsMemoryHandle GetGFXHandle() const override;

  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;

#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrContext* gr_context) override;

  bool ReadbackToMemory(const SkPixmap* pixmaps, uint32_t planes) override;
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      GrContext* skity_context) override;
#endif  // ENABLE_SKITY

 private:
  static fml::CFRef<IOSurfaceRef> CreateIOSurface(
      SharedImageBacking::PixelFormat pixel_format, skity::Vec2 size);

  fml::CFRef<IOSurfaceRef> io_surface_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_IOSURFACE_IMAGE_BACKING_H_
