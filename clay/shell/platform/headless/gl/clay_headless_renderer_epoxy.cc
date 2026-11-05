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

static void LogEglError(const std::string& reason) {
  const EGLint error = eglGetError();
  FML_LOG(ERROR) << "EGL: " << reason << " eglGetError returned " << error;
}

EGLDisplay GetSurfacelessDisplay() {
  auto get_platform_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
      eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (!get_platform_display) {
    return EGL_NO_DISPLAY;
  }
  return get_platform_display(EGL_PLATFORM_SURFACELESS_MESA,
                              EGL_DEFAULT_DISPLAY, nullptr);
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
      // Prefer Mesa's surfaceless platform so Linux headless rendering does not
      // depend on X11 or Wayland. This selects llvmpipe when the process uses
      // LIBGL_ALWAYS_SOFTWARE=1 and GALLIUM_DRIVER=llvmpipe.
      egl_display_ = GetSurfacelessDisplay();
      if (egl_display_ == EGL_NO_DISPLAY) {
        egl_display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      }
      if (egl_display_ == EGL_NO_DISPLAY) {
        LogEglError("Failed to get EGL display.");
        return false;
      }

      if (!eglInitialize(egl_display_, nullptr, nullptr)) {
        LogEglError("Failed to initialize EGL.");
        return false;
      }

      if (!epoxy_has_egl_extension(egl_display_,
                                   "EGL_KHR_surfaceless_context")) {
        LogEglError("EGL_KHR_surfaceless_context is not supported.");
        return false;
      }

      if (!epoxy_has_egl_extension(egl_display_, "EGL_KHR_image_base")) {
        LogEglError("EGL_KHR_image_base not supported.");
        return false;
      }
      if (!epoxy_has_egl_extension(egl_display_,
                                   "EGL_KHR_gl_texture_2D_image")) {
        LogEglError("EGL_KHR_gl_texture_2D_image not supported.");
        return false;
      }

#if defined(ENABLE_SKITY)
      // Skity's offscreen blit path requires OpenGL ES 3.
      constexpr EGLint kClientVersion = 3;
      constexpr EGLint kRenderableType = EGL_OPENGL_ES3_BIT_KHR;
#else
      constexpr EGLint kClientVersion = 2;
      constexpr EGLint kRenderableType = EGL_OPENGL_ES2_BIT;
#endif
      if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        LogEglError("Failed to bind the OpenGL ES API.");
        return false;
      }
      if (epoxy_has_egl_extension(egl_display_, "EGL_KHR_no_config_context")) {
        // Mesa's surfaceless platform may expose no EGLConfig at all. A
        // context without an EGLConfig is suitable when no EGLSurface is used;
        // all color/depth/stencil storage comes from Clay's GL framebuffers.
        egl_config_ = EGL_NO_CONFIG_KHR;
      } else {
        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE,
            kRenderableType,
            EGL_RED_SIZE,
            8,
            EGL_GREEN_SIZE,
            8,
            EGL_BLUE_SIZE,
            8,
            EGL_ALPHA_SIZE,
            8,
            EGL_NONE,
        };
        EGLint num_config = 0;
        if (eglChooseConfig(egl_display_, attribs, &egl_config_, 1,
                            &num_config) != EGL_TRUE ||
            num_config < 1) {
          LogEglError(
              "Failed to choose an EGL config and contexts without an "
              "EGLConfig are not supported.");
          return false;
        }
      }

      const EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, kClientVersion,
                                   EGL_NONE};
      egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                      attributes);
      if (egl_context_ == EGL_NO_CONTEXT) {
        LogEglError("Failed to create EGL context.");
        return false;
      }
    }

    if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       egl_context_) != EGL_TRUE) {
      LogEglError("Failed to make the Epoxy context current.");
      return false;
    }
    return true;
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
