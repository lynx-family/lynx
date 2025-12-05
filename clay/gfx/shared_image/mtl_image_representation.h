// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_MTL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_MTL_IMAGE_REPRESENTATION_H_

#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>

#include <memory>

#include "clay/fml/platform/darwin/cf_utils.h"
#include "clay/fml/platform/darwin/scoped_nsobject.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {
class SharedImageBacking;

class MTLStorageManager : public RepresentationStorageManager {
 public:
  explicit MTLStorageManager(id<MTLDevice> device);

  ~MTLStorageManager() override;

  CVMetalTextureRef CreateTextureFromStorage(CVPixelBufferRef pixel_buffer,
                                             MTLPixelFormat pixelFormat,
                                             size_t width, size_t height);

  void FlushStorageRecycle();

 private:
  fml::CFRef<CVMetalTextureCacheRef> cv_mtl_texture_cache_;
};

class API_AVAILABLE(macos(10.6), ios(11.0),
                    tvos(11.0)) MTLImageRepresentation final :

    public SharedImageRepresentation {
 public:
  MTLImageRepresentation(id<MTLDevice> device, id<MTLCommandQueue> queue,
                         fml::RefPtr<SharedImageBacking> backing);
  ~MTLImageRepresentation() override;

  ImageRepresentationType GetType() const override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool EndRead() override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndWrite() override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  fml::RefPtr<RepresentationStorageManager> GetTextureManager() const override;
  void SetTextureManager(fml::RefPtr<RepresentationStorageManager>) override;

 private:
  id<MTLTexture> GetMTLTexture();

  bool GetMTLTextureFromIOSurface();
  bool GetMTLTextureFromCVPixelBuffer();

  fml::scoped_nsprotocol<id<MTLDevice> > device_;
  fml::scoped_nsprotocol<id<MTLCommandQueue> > queue_;
  fml::scoped_nsprotocol<id> event_;  // MTLSharedEvent
  uint64_t event_value_ = 0;
  fml::scoped_nsprotocol<id<MTLTexture> > texture_;
  fml::RefPtr<MTLStorageManager> storage_manager_;
};
}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_MTL_IMAGE_REPRESENTATION_H_
