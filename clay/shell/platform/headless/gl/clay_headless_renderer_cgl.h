// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_CGL_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_CGL_H_

#import <OpenGL/OpenGL.h>

#include <optional>

#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {

class ClayHeadlessRendererCGL : public ClayHeadlessRendererSharedImageGL {
 public:
  ClayHeadlessRendererCGL(ClayHeadlessEngine* engine,
                          const ClayHardwareRendererConfig& config);

  ~ClayHeadlessRendererCGL() override;

  bool MakeCurrent() override;
  bool ClearCurrent() override;

 private:
  CGLContextObj gl_context_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_CGL_H_
