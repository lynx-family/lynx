// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_EAGL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_EAGL_IMAGE_REPRESENTATION_H_

#import <CoreVideo/CoreVideo.h>
#import <OpenGLES/EAGL.h>

#include <memory>
#include <optional>

#include "clay/fml/platform/darwin/cf_utils.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class CVPixelBufferImageBacking;

class EAGLStorageManager : public RepresentationStorageManager {
 public:
  explicit EAGLStorageManager(EAGLContext* gl_context);
  ~EAGLStorageManager() override;

  CVOpenGLESTextureRef CreateTextureFromStorage(CVPixelBufferRef pixel_buffer);
  void FlushStorageRecycle();

 private:
  fml::CFRef<CVOpenGLESTextureCacheRef> eagl_texture_cache_;
};

class EAGLImageRepresentation final : public GLImageRepresentation {
 public:
  explicit EAGLImageRepresentation(
      fml::RefPtr<CVPixelBufferImageBacking> backing);
  ~EAGLImageRepresentation() override;

  ImageRepresentationType GetType() const override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  fml::RefPtr<RepresentationStorageManager> GetTextureManager() const override;
  void SetTextureManager(fml::RefPtr<RepresentationStorageManager>) override;

 private:
  std::optional<TextureInfo> GetTexImage() override;
  bool ReleaseTexImage() override;
  std::optional<FramebufferInfo> BindFrameBuffer() override;
  bool UnbindFrameBuffer() override;

  bool GetTexImageFromCVPixelBuffer();

  EAGLContext* gl_context_ = nil;
  uint32_t texture_id_ = 0;
  uint32_t fbo_id_ = 0;
  uint32_t internal_format_ = 0;
  fml::RefPtr<EAGLStorageManager> storage_manager_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_EAGL_IMAGE_REPRESENTATION_H_
