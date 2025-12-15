// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_EPOXY_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_EPOXY_H_

#include <memory>
#include <optional>

#include "clay/shell/platform/headless/clay_headless_engine.h"
#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {

class ClayHeadlessEpoxyManager;

class ClayHeadlessRendererEpoxy : public ClayHeadlessRendererSharedImageGL {
 public:
  ClayHeadlessRendererEpoxy(ClayHeadlessEngine* engine,
                            const ClayHardwareRendererConfig& config);

  ~ClayHeadlessRendererEpoxy() override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

  bool MakeCurrent() override;
  bool ClearCurrent() override;

 private:
  std::unique_ptr<ClayHeadlessEpoxyManager> epoxy_manager_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_EPOXY_H_
