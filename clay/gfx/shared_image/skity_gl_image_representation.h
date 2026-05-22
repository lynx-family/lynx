// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_SKITY_GL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_SKITY_GL_IMAGE_REPRESENTATION_H_

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"
#include "skity/graphic/image.hpp"

namespace clay {

class SkityGLImageRepresentation final : public SkityImageRepresentation {
 public:
  SkityGLImageRepresentation(
      skity::GPUContext* skity_context,
      fml::RefPtr<GLImageRepresentation> gl_representation);
  ~SkityGLImageRepresentation() override;

  std::shared_ptr<SkityImage> GetSkityImage() override;
  bool EndRead() override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  fml::RefPtr<RepresentationStorageManager> GetTextureManager() const override;
  void SetTextureManager(
      fml::RefPtr<RepresentationStorageManager> storage_manager) override;

 private:
  skity::GPUContext* skity_context_;
  fml::RefPtr<GLImageRepresentation> gl_representation_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_SKITY_GL_IMAGE_REPRESENTATION_H_
