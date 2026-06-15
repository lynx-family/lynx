// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/embedder_surface_gl.h"

#include <memory>
#include <utility>

#include "clay/shell/platform/embedder/shell_platform_embedder_rendering_backend.h"

namespace clay {

EmbedderSurfaceGL::EmbedderSurfaceGL(GPUSurfaceGLDelegate* delegate)
    : delegate_(delegate) {
  valid_ = true;
}

EmbedderSurfaceGL::~EmbedderSurfaceGL() = default;

// |OutputSurface|
bool EmbedderSurfaceGL::IsValid() const { return valid_; }

// |GPUSurfaceGLDelegate|
std::unique_ptr<GLContextResult> EmbedderSurfaceGL::GLContextMakeCurrent() {
  return delegate_->GLContextMakeCurrent();
}

// |GPUSurfaceGLDelegate|
bool EmbedderSurfaceGL::GLContextClearCurrent() {
  return delegate_->GLContextClearCurrent();
}

void EmbedderSurfaceGL::GLContextSetDamageRegion(
    const std::optional<skity::Rect>& region) {
  delegate_->GLContextSetDamageRegion(region);
}

// |GPUSurfaceGLDelegate|
bool EmbedderSurfaceGL::GLContextPresent(const GLPresentInfo& present_info) {
  return delegate_->GLContextPresent(present_info);
}

// |GPUSurfaceGLDelegate|
GLFBOInfo EmbedderSurfaceGL::GLContextFBO(GLFrameInfo frame_info) const {
  return delegate_->GLContextFBO(frame_info);
}

// |GPUSurfaceGLDelegate|
bool EmbedderSurfaceGL::GLContextFBOResetAfterPresent() const {
  return delegate_->GLContextFBOResetAfterPresent();
}

// |GPUSurfaceGLDelegate|
skity::Matrix EmbedderSurfaceGL::GLContextSurfaceTransformation() const {
  return delegate_->GLContextSurfaceTransformation();
}

// |GPUSurfaceGLDelegate|
EmbedderSurfaceGL::GLProcResolver EmbedderSurfaceGL::GetGLProcResolver() const {
  return delegate_->GetGLProcResolver();
}

// |GPUSurfaceGLDelegate|
SurfaceFrame::FramebufferInfo EmbedderSurfaceGL::GLContextFramebufferInfo()
    const {
  return delegate_->GLContextFramebufferInfo();
}

// |OutputSurface|
std::unique_ptr<Surface> EmbedderSurfaceGL::CreateGPUSurface(
    clay::GrContext* context) {
#ifndef ENABLE_SKITY
  const bool render_to_surface = true;
  return std::make_unique<GPUSurfaceGLSkia>(
      context ? sk_ref_sp(context) : GetMainGrContext(),
      this,              // GPU surface GL delegate
      render_to_surface  // render to surface
  );
#else
  return std::make_unique<GPUSurfaceGLSkity>(
      this,
      context ? std::shared_ptr<clay::GrContext>(context) : GetMainGrContext());
#endif  // ENABLE_SKITY
}

// |OutputSurface|
clay::GrContextPtr EmbedderSurfaceGL::GetMainGrContext() {
  if (!main_context_) {
#ifndef ENABLE_SKITY
    main_context_ = GPUSurfaceGLSkia::MakeGLContext(this);
#else
    main_context_ = GPUSurfaceGLSkity::MakeGLContext(this);
#endif  // ENABLE_SKITY
  }
  return main_context_;
}

std::unique_ptr<GLContextResult> EmbedderSurfaceGL::MakeCurrent() {
  return GLContextMakeCurrent();
}

#ifdef ENABLE_SKITY
std::optional<OutputSurface::SkityPrecompileConfig>
EmbedderSurfaceGL::GetSkityPrecompileConfig() const {
  return SkityPrecompileConfig{skity::PrecompileColorType::kRGBA, false};
}
#endif

}  // namespace clay
