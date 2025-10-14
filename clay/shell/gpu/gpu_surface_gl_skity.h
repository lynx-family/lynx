// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_GPU_GPU_SURFACE_GL_SKITY_H_
#define CLAY_SHELL_GPU_GPU_SURFACE_GL_SKITY_H_

#include <functional>
#include <memory>
#include <unordered_map>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/common/graphics/gl_context_switch.h"
#include "clay/flow/embedded_views.h"
#include "clay/flow/surface.h"
#include "clay/shell/gpu/gpu_surface_gl_delegate.h"
#include "skity/gpu/gpu_context.hpp"
#include "skity/skity.hpp"

namespace clay {

class GPUSurfaceGLSkity : public Surface {
 public:
  static std::shared_ptr<skity::GPUContext> MakeGLContext(
      GPUSurfaceGLDelegate* delegate);

  GPUSurfaceGLSkity(GPUSurfaceGLDelegate* delegate,
                    std::shared_ptr<skity::GPUContext> skity_context);

  // |Surface|
  ~GPUSurfaceGLSkity() override;

  // |Surface|
  bool IsValid() override;

  // |Surface|
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override;

  // |Surface|
  skity::Matrix GetRootTransformation() const override;

  // |Surface|
  std::unique_ptr<GLContextResult> MakeRenderContextCurrent() override;

  // |Surface|
  bool ClearRenderContext() override;

  // |Surface|
  bool EnableRasterCache() const override;

  // |Surface|
  skity::GPUContext* GetContext() override;

 private:
  std::shared_ptr<skity::GPUSurface> AcquireRenderSurface(
      const skity::Vec2& size);
  bool PresentSurface(const SurfaceFrame& frame);

  GPUSurfaceGLDelegate* delegate_;
  bool valid_ = false;

  // Contains multiple GPUSurfaces (which counts on the number of buffers).
  // All the GPUSurfaces have the same size, but refer to different FBOs.
  // In FunctorView mode, there might be 2 GPUSurfaces. And In SurfaceView,
  // there will only be 1.
  std::unordered_map<uint32_t, std::shared_ptr<skity::GPUSurface>>
      gpu_surface_map_;
  // Refer to the current GPUSurface in use.
  std::shared_ptr<skity::GPUSurface> gpu_surface_;
  std::shared_ptr<skity::GPUContext> gpu_context_;
  uint32_t fbo_id_ = 0;
  skity::Vec2 size_;

  // The current FBO's existing damage, as tracked by the GPU surface, delegates
  // still have an option of overriding this damage with their own in
  // `GLContextFrameBufferInfo`.
  std::optional<skity::Rect> existing_damage_ = std::nullopt;

  // WeakPtrFactory must be the last member.
  fml::WeakPtrFactory<GPUSurfaceGLSkity> weak_factory_;
  BASE_DISALLOW_COPY_AND_ASSIGN(GPUSurfaceGLSkity);
};

}  // namespace clay

#endif  // CLAY_SHELL_GPU_GPU_SURFACE_GL_SKITY_H_
