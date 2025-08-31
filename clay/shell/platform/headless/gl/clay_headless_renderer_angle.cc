// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_angle.h"

#include <Windows.h>

#include <string>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/utils/d3d11_device_creator.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

// Logs an EGL error to stderr. This automatically calls eglGetError()
// and logs the error code.
static void LogEglError(std::string message) {
  EGLint error = eglGetError();
  FML_LOG(ERROR) << "EGL:" << message
                 << ", EGL: eglGetError returned: " << error;
}

std::unique_ptr<HeadlessAngleSurfaceManager>
HeadlessAngleSurfaceManager::Create() {
  return std::unique_ptr<HeadlessAngleSurfaceManager>(
      new HeadlessAngleSurfaceManager());
}

HeadlessAngleSurfaceManager::HeadlessAngleSurfaceManager()
    : egl_config_(nullptr),
      egl_display_(EGL_NO_DISPLAY),
      egl_context_(EGL_NO_CONTEXT),
      egl_device_(nullptr) {
  initialize_succeeded_ = Initialize();
}

HeadlessAngleSurfaceManager::~HeadlessAngleSurfaceManager() { CleanUp(); }

bool HeadlessAngleSurfaceManager::MakeCurrent() {
  return (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_,
                         egl_context_) == EGL_TRUE);
}

bool HeadlessAngleSurfaceManager::ClearContext() {
  return (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT) == EGL_TRUE);
}

Microsoft::WRL::ComPtr<ID3D11Device> HeadlessAngleSurfaceManager::GetDevice() {
  if (resolved_device_) {
    return resolved_device_;
  }
  Microsoft::WRL::ComPtr<ID3D11Device> result;
  PFNEGLQUERYDISPLAYATTRIBEXTPROC egl_query_display_attrib_EXT =
      reinterpret_cast<PFNEGLQUERYDISPLAYATTRIBEXTPROC>(
          eglGetProcAddress("eglQueryDisplayAttribEXT"));

  PFNEGLQUERYDEVICEATTRIBEXTPROC egl_query_device_attrib_EXT =
      reinterpret_cast<PFNEGLQUERYDEVICEATTRIBEXTPROC>(
          eglGetProcAddress("eglQueryDeviceAttribEXT"));

  if (!egl_query_display_attrib_EXT || !egl_query_device_attrib_EXT) {
    return result;
  }

  EGLAttrib egl_device = 0;
  EGLAttrib angle_device = 0;
  if (egl_query_display_attrib_EXT(egl_display_, EGL_DEVICE_EXT, &egl_device) ==
      EGL_TRUE) {
    if (egl_query_device_attrib_EXT(reinterpret_cast<EGLDeviceEXT>(egl_device),
                                    EGL_D3D11_DEVICE_ANGLE,
                                    &angle_device) == EGL_TRUE) {
      result = reinterpret_cast<ID3D11Device*>(angle_device);
    }
  }

  resolved_device_ = result;
  return result;
}

bool HeadlessAngleSurfaceManager::TryInitializeD3D11Device() {
  d3d11_ = fml::NativeLibrary::Create("d3d11.dll");

  if (!d3d11_) {
    FML_LOG(ERROR) << "HeadlessAngleSurfaceManager::TryInitializeD3D11Device, "
                      "Could not load D3D11 library.";
    return false;
  }

  std::optional<PFN_D3D11_CREATE_DEVICE> D3D11CreateDevice =
      d3d11_->ResolveFunction<PFN_D3D11_CREATE_DEVICE>("D3D11CreateDevice");
  if (FAILED(CreateSafeD3D11Device(D3D11CreateDevice, &resolved_device_,
                                   nullptr))) {
    FML_LOG(WARNING) << "Could not create D3D11 Device.";
    return false;
  }

  egl_device_ = eglCreateDeviceANGLE(EGL_D3D11_DEVICE_ANGLE,
                                     resolved_device_.Get(), nullptr);
  if (!egl_device_) {
    LogEglError(
        "HeadlessAngleSurfaceManager::TryInitializeD3D11Device, Could not "
        "create EGL device.");
    return false;
  }

  FML_LOG(ERROR) << "HeadlessAngleSurfaceManager::TryInitializeD3D11Device "
                    "success.";
  return true;
}

bool HeadlessAngleSurfaceManager::Initialize() {
#ifndef CLAY_FORCE_D3D9
  FML_LOG(INFO)
      << "HeadlessAngleSurfaceManager::Initialize, TryInitializeD3D11Device.";
  TryInitializeD3D11Device();
#endif

  // TODO(dnfield): Enable MSAA here, see similar code in android_context_gl.cc
  // Will need to plumb in argument from project bundle for sampling rate.
  // https://github.com/flutter/flutter/issues/100392
  const EGLint config_attributes[] = {EGL_RED_SIZE,   8, EGL_GREEN_SIZE,   8,
                                      EGL_BLUE_SIZE,  8, EGL_ALPHA_SIZE,   8,
                                      EGL_DEPTH_SIZE, 8, EGL_STENCIL_SIZE, 8,
                                      EGL_NONE};

  const EGLint display_context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                               EGL_NONE};

  // These are preferred display attributes and request ANGLE's D3D11
  // renderer. eglInitialize will only succeed with these attributes if the
  // hardware supports D3D11 Feature Level 10_0+.
  const EGLint d3d11_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

      // EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that will
      // enable ANGLE to automatically call the IDXGIDevice3::Trim method on
      // behalf of the application when it gets suspended.
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,

      // This extension allows angle to render directly on a D3D swapchain
      // in the correct orientation on D3D11.
      EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE,
      EGL_EXPERIMENTAL_PRESENT_PATH_FAST_ANGLE,

      // Request high-performance GPU (discrete GPU) instead of integrated GPU.
      EGL_POWER_PREFERENCE_ANGLE,
      EGL_HIGH_POWER_ANGLE,

      EGL_NONE,
  };

  // These are used to request ANGLE's D3D11 renderer, with D3D11 Feature
  // Level 9_3.
  const EGLint d3d11_fl_9_3_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
      9,
      EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE,
      3,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };

  // These attributes request D3D11 WARP (software rendering fallback) in case
  // hardware-backed D3D11 is unavailable.
  const EGLint d3d11_warp_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };

  // These are used to request ANGLE's D3D9 renderer as a fallback if D3D11
  // is not available.
  const EGLint d3d9_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
      EGL_NONE,
  };

  std::vector<const EGLint*> display_attributes_configs = {
#ifndef CLAY_FORCE_D3D9
      d3d11_display_attributes,
      d3d11_fl_9_3_display_attributes,
      d3d11_warp_display_attributes,
#endif
      d3d9_display_attributes,
  };

  PFNEGLGETPLATFORMDISPLAYEXTPROC egl_get_platform_display_EXT =
      reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
          eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (!egl_get_platform_display_EXT) {
    LogEglError("eglGetPlatformDisplayEXT not available");
    return false;
  }

  // Attempt to initialize ANGLE's renderer in order of: D3D11, D3D11 Feature
  // Level 9_3 and finally D3D11 WARP.
  for (auto config : display_attributes_configs) {
    bool should_log = (config == display_attributes_configs.back());
    if (InitializeEGL(egl_get_platform_display_EXT, config, should_log)) {
      break;
    }
  }

  EGLint numConfigs = 0;
  if ((eglChooseConfig(egl_display_, config_attributes, &egl_config_, 1,
                       &numConfigs) == EGL_FALSE) ||
      (numConfigs == 0)) {
    LogEglError("Failed to choose first context");
    return false;
  }

  egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                  display_context_attributes);
  if (egl_context_ == EGL_NO_CONTEXT) {
    LogEglError("Failed to create EGL context");
    return false;
  } else {
    FML_LOG(INFO) << "HeadlessAngleSurfaceManager::Initialize, "
                     "eglCreateContext success.";
  }

  // We only ever create pbuffer surfaces for background resource loading
  // contexts. We never bind the pbuffer to anything.
  const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};

  egl_surface_ = eglCreatePbufferSurface(egl_display_, egl_config_, attribs);

  if (egl_surface_ == EGL_NO_SURFACE) {
    LogEglError("Failed to create EGL surface");
    return false;
  }

  return true;
}

void HeadlessAngleSurfaceManager::CleanUp() {
  EGLBoolean result = EGL_FALSE;

  // Needs to be reset before destroying the EGLContext.
  resolved_device_.Reset();

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
    if (egl_device_ != nullptr) {
      // If the display is created from an owned device,
      // it needs to be released.
      result = eglTerminate(egl_display_);
      if (result == EGL_FALSE) {
        LogEglError("Failed to destroy display");
      }
    }
    egl_display_ = EGL_NO_DISPLAY;
  }

  if (egl_device_ != nullptr) {
    result = eglReleaseDeviceANGLE(egl_device_);
    egl_device_ = nullptr;
    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy device");
    }
  }
}

bool HeadlessAngleSurfaceManager::InitializeEGL(
    PFNEGLGETPLATFORMDISPLAYEXTPROC egl_get_platform_display_EXT,
    const EGLint* config, bool should_log) {
  if (egl_device_ != nullptr) {
    egl_display_ = egl_get_platform_display_EXT(EGL_PLATFORM_DEVICE_EXT,
                                                egl_device_, config);
  } else {
    egl_display_ = egl_get_platform_display_EXT(EGL_PLATFORM_ANGLE_ANGLE,
                                                EGL_DEFAULT_DISPLAY, config);
  }

  if (egl_display_ == EGL_NO_DISPLAY) {
    if (should_log) {
      LogEglError("Failed to get a compatible EGLdisplay");
    }
    return false;
  }

  if (eglInitialize(egl_display_, nullptr, nullptr) == EGL_FALSE) {
    if (should_log) {
      LogEglError("Failed to initialize EGL via ANGLE");
    }
    return false;
  }

  return true;
}

ClayHeadlessRendererAngle::ClayHeadlessRendererAngle(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererSharedImageGL(engine, config) {
  surface_manager_ = HeadlessAngleSurfaceManager::Create();
}

ClayHeadlessRendererAngle::~ClayHeadlessRendererAngle() = default;

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererAngle::GetGLProcResolver() const {
  return [](const char* name) -> void* {
    return reinterpret_cast<void*>(eglGetProcAddress(name));
  };
}

bool ClayHeadlessRendererAngle::MakeCurrent() {
  return surface_manager_->MakeCurrent();
}

bool ClayHeadlessRendererAngle::ClearCurrent() {
  return surface_manager_->ClearContext();
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererAngle>(engine, config);
}

}  // namespace clay
