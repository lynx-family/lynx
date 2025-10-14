// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_EXAMPLE_GLFW_SURFACE_GL_IMPL_H_
#define CLAY_EXAMPLE_GLFW_SURFACE_GL_IMPL_H_

#include <memory>

#include "build/build_config.h"
#include "clay/public/clay.h"
#include "clay/shell/common/output_surface.h"
#include "clay/shell/gpu/gpu_surface_gl_skia.h"
#include "third_party/skia/src/gpu/gl/GrGLDefines.h"  // nogncheck

namespace clay {
namespace example {

class SurfaceDelegate {
 public:
  virtual bool MakeCurrent() = 0;
  virtual bool ClearCurrent() = 0;
  virtual bool Present() = 0;
  virtual uint32_t FBO(const ClayFrameInfo* frame_info) = 0;
  virtual void* GetGLProcResolver(const char* what) const = 0;
};

class SurfaceGLImpl : public clay::OutputSurface,
                      public clay::GPUSurfaceGLDelegate {
 public:
  explicit SurfaceGLImpl(SurfaceDelegate* delegate) : delegate_(delegate) {}
  ~SurfaceGLImpl() override {}

 private:
  // |OutputSurface|
  bool IsValid() const override { return true; }

  // |OutputSurface|
  std::unique_ptr<clay::Surface> CreateGPUSurface(
      clay::GrContext* context) override {
    const bool render_to_surface = true;
    return std::make_unique<clay::GPUSurfaceGLSkia>(
        context ? sk_ref_sp(context) : GetMainGrContext(),
        this,              // GPU surface GL delegate
        render_to_surface  // render to surface
    );
  }

  // |OutputSurface|
  clay::GrContextPtr GetMainGrContext() override {
    if (!main_context_) {
      main_context_ = clay::GPUSurfaceGLSkia::MakeGLContext(this);
    }
    return main_context_;
  }

  // |GPUSurfaceGLDelegate|
  std::unique_ptr<GLContextResult> GLContextMakeCurrent() override {
    return std::make_unique<GLContextDefaultResult>(delegate_->MakeCurrent());
  }

  // |GPUSurfaceGLDelegate|
  bool GLContextClearCurrent() override { return delegate_->ClearCurrent(); }

  // |GPUSurfaceGLDelegate|
  bool GLContextPresent(const clay::GLPresentInfo& present_info) override {
    delegate_->Present();
    return true;
  }

  // |GPUSurfaceGLDelegate|
  GLFBOInfo GLContextFBO(GLFrameInfo gl_frame_info) const override {
    ClayFrameInfo frame_info = {};
    frame_info.struct_size = sizeof(ClayFrameInfo);
    frame_info.width = gl_frame_info.width;
    frame_info.height = gl_frame_info.height;
    return GLFBOInfo{.fbo_id = delegate_->FBO(&frame_info),
                     .existing_damage = {}};
  }

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override {
    return [&](const char* name) -> void* {
      return reinterpret_cast<void*>(delegate_->GetGLProcResolver(name));
    };
  }

  bool GLContextFBOResetAfterPresent() const override { return true; }

  SurfaceDelegate* delegate_;
  clay::GrContextPtr main_context_;
};

}  // namespace example
}  // namespace clay

#endif  // CLAY_EXAMPLE_GLFW_SURFACE_GL_IMPL_H_
