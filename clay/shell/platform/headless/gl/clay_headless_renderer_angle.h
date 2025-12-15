// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_ANGLE_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_ANGLE_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <memory>

#include "base/include/fml/macros.h"
#include "clay/fml/native_library.h"
#include "clay/shell/platform/headless/gl/clay_headless_renderer_gl.h"

namespace clay {

class HeadlessAngleSurfaceManager {
 public:
  static std::unique_ptr<HeadlessAngleSurfaceManager> Create();

  ~HeadlessAngleSurfaceManager();

  bool MakeCurrent();

  bool ClearContext();

  // The current D3D device.
  Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();

  BASE_DISALLOW_COPY_AND_ASSIGN(HeadlessAngleSurfaceManager);

 private:
  // Creates a new surface manager retaining reference to the passed-in target
  // for the lifetime of the manager.
  HeadlessAngleSurfaceManager();

  bool TryInitializeD3D11Device();
  bool Initialize();
  void CleanUp();

  bool InitializeEGL(
      PFNEGLGETPLATFORMDISPLAYEXTPROC egl_get_platform_display_EXT,
      const EGLint* config, bool should_log);

  EGLDeviceEXT egl_device_;

  EGLDisplay egl_display_;

  EGLContext egl_context_;

  EGLConfig egl_config_;

  // Pbuffer surface to make current
  EGLSurface egl_surface_ = EGL_NO_SURFACE;

  bool initialize_succeeded_;

  Microsoft::WRL::ComPtr<ID3D11Device> resolved_device_;

  fml::RefPtr<fml::NativeLibrary> d3d11_;
};

class ClayHeadlessRendererAngle final
    : public ClayHeadlessRendererSharedImageGL {
 public:
  explicit ClayHeadlessRendererAngle(ClayHeadlessEngine* engine,
                                     const ClayHardwareRendererConfig& config);
  ~ClayHeadlessRendererAngle() override;

  // |GPUSurfaceGLDelegate|
  GLProcResolver GetGLProcResolver() const override;

  bool MakeCurrent() override;
  bool ClearCurrent() override;

 private:
  std::unique_ptr<HeadlessAngleSurfaceManager> surface_manager_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_GL_CLAY_HEADLESS_RENDERER_ANGLE_H_
