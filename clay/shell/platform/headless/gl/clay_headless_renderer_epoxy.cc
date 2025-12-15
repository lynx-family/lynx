// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_epoxy.h"

#include <epoxy/egl.h>
#include <epoxy/gl.h>

#include <memory>
#include <string>

#include "clay/fml/logging.h"

namespace clay {

namespace {

static void LogEglError(std::string message) {
  EGLint error = eglGetError();
  FML_LOG(ERROR) << "EGL: " << message;
  FML_LOG(ERROR) << "EGL: eglGetError returned " << error;
}

}  // namespace

class ClayHeadlessEpoxyManager {
 public:
  ClayHeadlessEpoxyManager() = default;

  ~ClayHeadlessEpoxyManager() {
    if (egl_context_ != EGL_NO_CONTEXT && egl_display_ != EGL_NO_DISPLAY) {
      eglDestroyContext(egl_display_, egl_context_);
      eglTerminate(egl_display_);
    }
  }

  bool MakeCurrent() {
    if (egl_context_ == EGL_NO_CONTEXT) {
      egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      if (egl_display_ == EGL_NO_DISPLAY) {
        LogEglError("Failed to get EGL display.");
        return false;
      }

      EGLint major, minor;
      if (!eglInitialize(egl_display_, &major, &minor)) {
        LogEglError("Failed to initialize EGL.");
        return false;
      }

      if (!epoxy_has_egl_extension(egl_display_, "EGL_KHR_image_base")) {
        LogEglError("EGL_KHR_image_base not supported.");
        return false;
      }

      const EGLint attribs[] = {EGL_RED_SIZE,   8,  EGL_GREEN_SIZE,   8,
                                EGL_BLUE_SIZE,  8,  EGL_ALPHA_SIZE,   8,
                                EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
                                EGL_NONE};
      EGLint num_config;
      if (!eglChooseConfig(egl_display_, attribs, &egl_config_, 1,
                           &num_config)) {
        LogEglError("Failed to choose EGL config.");
        return false;
      }

      EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
      egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                      attributes);
      if (egl_context_ == EGL_NO_CONTEXT) {
        LogEglError("Failed to create EGL context.");
        return false;
      }
    }

    return eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          egl_context_) == EGL_TRUE;
  }
  bool ClearCurrent() {
    return eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          EGL_NO_CONTEXT) == EGL_TRUE;
  }

 private:
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLConfig egl_config_ = nullptr;
};

ClayHeadlessRendererEpoxy::ClayHeadlessRendererEpoxy(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererSharedImageGL(engine, config),
      epoxy_manager_(std::make_unique<ClayHeadlessEpoxyManager>()) {}

ClayHeadlessRendererEpoxy::~ClayHeadlessRendererEpoxy() = default;

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererEpoxy::GetGLProcResolver() const {
  return [](const char* name) -> void* {
    return reinterpret_cast<void*>(eglGetProcAddress(name));
  };
}

bool ClayHeadlessRendererEpoxy::MakeCurrent() {
  return epoxy_manager_->MakeCurrent();
}

bool ClayHeadlessRendererEpoxy::ClearCurrent() {
  return epoxy_manager_->ClearCurrent();
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererEpoxy>(engine, config);
}

}  // namespace clay
