// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SKIA_GL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SKIA_GL_IMAGE_REPRESENTATION_H_

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class SkiaGLImageRepresentation final : public SkiaImageRepresentation {
 public:
  SkiaGLImageRepresentation(
      GrDirectContext* gr_context,
      fml::RefPtr<GLImageRepresentation> gl_representation);
  ~SkiaGLImageRepresentation() override;

  sk_sp<SkImage> GetSkImage() override;
  bool EndRead() override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  fml::RefPtr<RepresentationStorageManager> GetTextureManager() const override;
  void SetTextureManager(
      fml::RefPtr<RepresentationStorageManager> storage_manager) override;

 private:
  GrDirectContext* gr_context_;
  fml::RefPtr<GLImageRepresentation> gl_representation_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SKIA_GL_IMAGE_REPRESENTATION_H_
