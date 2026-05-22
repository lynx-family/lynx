// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_NATIVE_IMAGE_EGL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_NATIVE_IMAGE_EGL_IMAGE_REPRESENTATION_H_

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class NativeImageImageBacking;

class NativeImageEGLImageRepresentation final : public GLImageRepresentation {
 public:
  explicit NativeImageEGLImageRepresentation(
      fml::RefPtr<NativeImageImageBacking> backing);

  ~NativeImageEGLImageRepresentation() override;

  ImageRepresentationType GetType() const override;
  void ConsumeFence(std::unique_ptr<FenceSync>) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

 private:
  std::optional<TextureInfo> GetTexImage() override;
  bool ReleaseTexImage() override;
  std::optional<FramebufferInfo> BindFrameBuffer() override;
  bool UnbindFrameBuffer() override;

  fml::RefPtr<NativeImageImageBacking> backing_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_NATIVE_IMAGE_EGL_IMAGE_REPRESENTATION_H_
