// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_swiftshader.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>

#include <memory>
#include <string>

#include "angle_gl.h"
#include "clay/fml/logging.h"

namespace clay {

namespace {

void LogEglError(const std::string& message) {
  FML_LOG(ERROR) << "SwiftShader EGL: " << message
                 << ", eglGetError: " << eglGetError();
}

}  // namespace

class ClayHeadlessSwiftShaderManager {
 public:
  ClayHeadlessSwiftShaderManager() = default;

  ~ClayHeadlessSwiftShaderManager() { CleanUp(); }

  bool MakeCurrent() {
    if (!EnsureInitialized()) {
      return false;
    }
    if (eglMakeCurrent(display_, surface_, surface_, context_) != EGL_TRUE) {
      LogEglError("Failed to make the context current");
      return false;
    }
    if (!renderer_logged_) {
      const auto* renderer = glGetString(GL_RENDERER);
      FML_LOG(INFO) << "Skity software rendering GL renderer: "
                    << (renderer ? reinterpret_cast<const char*>(renderer)
                                 : "unknown");
      renderer_logged_ = true;
    }
    return true;
  }

  bool ClearCurrent() {
    if (!initialized_) {
      return true;
    }
    return eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          EGL_NO_CONTEXT) == EGL_TRUE;
  }

 private:
  bool EnsureInitialized() {
    if (initialization_attempted_) {
      return initialized_;
    }
    initialization_attempted_ = true;
    initialized_ = Initialize();
    if (!initialized_) {
      CleanUp();
    }
    return initialized_;
  }

  bool Initialize() {
    const EGLAttrib display_attributes[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE,
        EGL_NONE,
    };
    display_ = eglGetPlatformDisplay(
        EGL_PLATFORM_ANGLE_ANGLE, reinterpret_cast<void*>(EGL_DEFAULT_DISPLAY),
        display_attributes);
    if (display_ == EGL_NO_DISPLAY) {
      LogEglError("Failed to get the ANGLE SwiftShader display");
      return false;
    }

    EGLint major = 0;
    EGLint minor = 0;
    if (eglInitialize(display_, &major, &minor) != EGL_TRUE) {
      LogEglError("Failed to initialize the ANGLE SwiftShader display");
      return false;
    }

    const EGLint config_attributes[] = {
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,
        EGL_PBUFFER_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_DEPTH_SIZE,
        0,
        EGL_STENCIL_SIZE,
        8,
        EGL_NONE,
    };
    EGLint config_count = 0;
    if (eglChooseConfig(display_, config_attributes, &config_, 1,
                        &config_count) != EGL_TRUE ||
        config_count == 0) {
      LogEglError("Failed to choose an EGL config");
      return false;
    }

    const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                         EGL_NONE};
    context_ =
        eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attributes);
    if (context_ == EGL_NO_CONTEXT) {
      LogEglError("Failed to create an EGL context");
      return false;
    }

    const EGLint surface_attributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    surface_ = eglCreatePbufferSurface(display_, config_, surface_attributes);
    if (surface_ == EGL_NO_SURFACE) {
      LogEglError("Failed to create an EGL pbuffer surface");
      return false;
    }
    return true;
  }

  void CleanUp() {
    if (display_ == EGL_NO_DISPLAY) {
      return;
    }
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(display_, surface_);
      surface_ = EGL_NO_SURFACE;
    }
    if (context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(display_, context_);
      context_ = EGL_NO_CONTEXT;
    }
    eglTerminate(display_);
    display_ = EGL_NO_DISPLAY;
  }

  bool initialization_attempted_ = false;
  bool initialized_ = false;
  bool renderer_logged_ = false;
  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLConfig config_ = nullptr;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;
};

ClayHeadlessRendererSwiftShader::ClayHeadlessRendererSwiftShader(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererSharedImageGL(engine, config),
      surface_manager_(std::make_unique<ClayHeadlessSwiftShaderManager>()) {}

ClayHeadlessRendererSwiftShader::~ClayHeadlessRendererSwiftShader() = default;

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererSwiftShader::GetGLProcResolver() const {
  return [](const char* name) -> void* {
    return reinterpret_cast<void*>(eglGetProcAddress(name));
  };
}

bool ClayHeadlessRendererSwiftShader::MakeCurrent() {
  return surface_manager_->MakeCurrent();
}

bool ClayHeadlessRendererSwiftShader::ClearCurrent() {
  return surface_manager_->ClearCurrent();
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererSwiftShader>(engine, config);
}

}  // namespace clay
