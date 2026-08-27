// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_SWIFTSHADER_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_SWIFTSHADER_H_

#include <memory>

#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {

class ClayHeadlessSwiftShaderManager;

class ClayHeadlessRendererSwiftShader final
    : public ClayHeadlessRendererSharedImageGL {
 public:
  ClayHeadlessRendererSwiftShader(ClayHeadlessEngine* engine,
                                  const ClayHardwareRendererConfig& config);
  ~ClayHeadlessRendererSwiftShader() override;

  GLProcResolver GetGLProcResolver() const override;

  bool MakeCurrent() override;
  bool ClearCurrent() override;

 private:
  std::unique_ptr<ClayHeadlessSwiftShaderManager> surface_manager_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_SWIFTSHADER_H_
