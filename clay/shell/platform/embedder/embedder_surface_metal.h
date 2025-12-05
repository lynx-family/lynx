// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_METAL_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_METAL_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/shell/gpu/gpu_surface_metal_delegate.h"
#include "clay/shell/platform/embedder/embedder_surface.h"

namespace clay {

class EmbedderSurfaceMetalDelegate {
 public:
  virtual ~EmbedderSurfaceMetalDelegate() = default;

  virtual GPUMTLDeviceHandle GetMTLDevice() const = 0;

  virtual GPUMTLCommandQueueHandle GetMTLCommandQueue() const = 0;

  virtual GPUMTLTextureInfo GetMTLTexture(
      const skity::Vec2& frame_size) const = 0;

  virtual bool PresentTexture(GPUMTLTextureInfo texture) const = 0;

  virtual bool EnablePartialRepaint() const = 0;
};

class EmbedderSurfaceMetal final : public EmbedderSurface,
                                   public GPUSurfaceMetalDelegate {
 public:
  explicit EmbedderSurfaceMetal(EmbedderSurfaceMetalDelegate* delegate);

  ~EmbedderSurfaceMetal() override;

 private:
  EmbedderSurfaceMetalDelegate* delegate_;
  bool valid_ = false;
  clay::GrContextPtr main_context_;

  // |OutputSurface|
  bool IsValid() const override;

  // |OutputSurface|
  std::unique_ptr<Surface> CreateGPUSurface(clay::GrContext* context) override;

  // |OutputSurface|
  clay::GrContextPtr GetMainGrContext() override;

  // |GPUSurfaceMetalDelegate|
  GPUCAMetalLayerHandle GetCAMetalLayer(
      const skity::Vec2& frame_size) const override;

  // |GPUSurfaceMetalDelegate|
  GPUMTLTextureInfo GetMTLTexture(const skity::Vec2& frame_size) const override;

  // |GPUSurfaceMetalDelegate|
  bool PresentTexture(GPUMTLTextureInfo texture) const override;

  // |GPUSurfaceMetalDelegate|
  bool EnablePartialRepaint() const override;

  BASE_DISALLOW_COPY_AND_ASSIGN(EmbedderSurfaceMetal);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_METAL_H_
