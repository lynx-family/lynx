// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_GL_H_
#define CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_GL_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/shell/gpu/gpu_surface_gl_delegate.h"
#include "clay/shell/platform/embedder/embedder_surface.h"

namespace clay {

class EmbedderSurfaceGL final : public EmbedderSurface,
                                public GPUSurfaceGLDelegate {
 public:
  explicit EmbedderSurfaceGL(GPUSurfaceGLDelegate* delegate);

  ~EmbedderSurfaceGL() override;

 private:
  bool valid_ = false;
  GPUSurfaceGLDelegate* delegate_;

  clay::GrContextPtr main_context_;

  // |OutputSurface|
  bool IsValid() const override;

  // |OutputSurface|
  std::unique_ptr<Surface> CreateGPUSurface(clay::GrContext*) override;

  // |OutputSurface|
  clay::GrContextPtr GetMainGrContext() override;

  // |GPUSurfaceGLDelegate|
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override;

  // |OutputSurface|
  std::unique_ptr<GLContextResult> MakeCurrent() override;

#ifdef ENABLE_SKITY
  // |OutputSurface|
  std::optional<SkityPrecompileConfig> GetSkityPrecompileConfig()
      const override;
#endif

  // |GPUSurfaceGLDelegate|
  bool GLContextClearCurrent() override;

  // |GPUSurfaceGLDelegate|
  void GLContextSetDamageRegion(
      const std::optional<skity::Rect>& region) override;

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const GLPresentInfo& present_info) override;

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo frame_info) const override;

  // |GPUSurfaceGLDelegate|
  bool GLContextFBOResetAfterPresent() const override;

  // |GPUSurfaceGLDelegate|
  skity::Matrix GLContextSurfaceTransformation() const override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

  // |GPUSurfaceGLDelegate|
  SurfaceFrame::FramebufferInfo GLContextFramebufferInfo() const override;

  BASE_DISALLOW_COPY_AND_ASSIGN(EmbedderSurfaceGL);
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_EMBEDDER_EMBEDDER_SURFACE_GL_H_
