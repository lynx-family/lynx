// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_gles.h"

#include <string>
#include <vector>

#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

namespace {
// Logs an EGL error to stderr. This automatically calls eglGetError()
// and logs the error code.
static void LogEglError(std::string message) {
  EGLint error = eglGetError();
  FML_LOG(ERROR) << "EGL: " << message;
  FML_LOG(ERROR) << "EGL: eglGetError returned " << error;
}

static bool ChooseEGLConfiguration(EGLDisplay display, EGLConfig* egl_config) {
  EGLint attributes[] = {
      // clang-format off
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      8,
      EGL_DEPTH_SIZE,      0,
      EGL_STENCIL_SIZE,    0,
      EGL_NONE,            // termination sentinel
      // clang-format on
  };
  EGLint config_count = 0;
  if (eglChooseConfig(display, attributes, egl_config, 1, &config_count) !=
      EGL_TRUE) {
    return false;
  }
  return config_count > 0 && egl_config != nullptr;
}

}  // namespace

ClayHeadlessRendererGLES::ClayHeadlessRendererGLES(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererSharedImageGL(engine, config),
      egl_display_(EGL_NO_DISPLAY),
      egl_context_(EGL_NO_CONTEXT),
      egl_config_(nullptr),
      egl_surface_(EGL_NO_SURFACE) {
  // Get the display.
  egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (egl_display_ == EGL_NO_DISPLAY) {
    LogEglError("Failed to get EGL display");
    return;
  }
  // Initialize the display connection.
  if (eglInitialize(egl_display_, nullptr, nullptr) != EGL_TRUE) {
    LogEglError("Failed to initialize EGL");
    egl_display_ = EGL_NO_DISPLAY;
    return;
  }
  // Choose an EGL configuration.
  if (!ChooseEGLConfiguration(egl_display_, &egl_config_)) {
    return;
  }
  const EGLint display_context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                               EGL_NONE};
  egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                  display_context_attributes);
  if (egl_context_ == EGL_NO_CONTEXT) {
    LogEglError("Failed to create EGL context");
    return;
  }
  // Create a 1x1 PbufferSurface to initialize the EGL context.
  const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  egl_surface_ = eglCreatePbufferSurface(egl_display_, egl_config_, attribs);
  if (egl_surface_ == EGL_NO_SURFACE) {
    LogEglError("Failed to create EGL surface");
    return;
  }
}

ClayHeadlessRendererGLES::~ClayHeadlessRendererGLES() {
  EGLBoolean result = EGL_FALSE;
  if (egl_display_ != EGL_NO_DISPLAY && egl_surface_ != EGL_NO_SURFACE) {
    result = eglDestroySurface(egl_display_, egl_surface_);
    egl_surface_ = EGL_NO_SURFACE;
    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy context");
    }
  }

  if (egl_display_ != EGL_NO_DISPLAY && egl_context_ != EGL_NO_CONTEXT) {
    result = eglDestroyContext(egl_display_, egl_context_);
    egl_context_ = EGL_NO_CONTEXT;

    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy context");
    }
  }

  if (egl_display_ != EGL_NO_DISPLAY) {
    result = eglTerminate(egl_display_);
    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy display");
    }
    egl_display_ = EGL_NO_DISPLAY;
  }
}

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererGLES::GetGLProcResolver() const {
  return [](const char* name) -> void* {
    return reinterpret_cast<void*>(eglGetProcAddress(name));
  };
}

bool ClayHeadlessRendererGLES::MakeCurrent() {
  return (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_,
                         egl_context_) == EGL_TRUE);
}

bool ClayHeadlessRendererGLES::ClearCurrent() {
  return (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT) == EGL_TRUE);
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererGLES>(engine, config);
}

}  // namespace clay
