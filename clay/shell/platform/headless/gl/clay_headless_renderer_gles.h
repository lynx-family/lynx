// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_GLES_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_GLES_H_

#include <EGL/egl.h>

#include <memory>

#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {

class ClayHeadlessRendererGLES final
    : public ClayHeadlessRendererSharedImageGL {
 public:
  ClayHeadlessRendererGLES(ClayHeadlessEngine* engine,
                           const ClayHardwareRendererConfig& config);
  ~ClayHeadlessRendererGLES() override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

  bool MakeCurrent() override;
  bool ClearCurrent() override;

 private:
  EGLDisplay egl_display_;

  EGLContext egl_context_;

  EGLConfig egl_config_;

  // Pbuffer surface to make current
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_GLES_H_
