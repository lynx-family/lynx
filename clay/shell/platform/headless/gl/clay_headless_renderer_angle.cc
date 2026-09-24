// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_angle.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <Windows.h>
#include <dxgi.h>

#include <array>
#include <sstream>
#include <string>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/utils/d3d11_device_creator.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

constexpr std::array<DWORD, 1> kDefaultD3D11DeviceRetryDelaysMs = {1000};
constexpr std::array<DWORD, 3> kDefaultAngleInitRetryDelaysMs = {1000, 2000,
                                                                 4000};

template <typename T>
static std::string ToHexString(T value) {
  std::ostringstream stream;
  stream << "0x" << std::hex << static_cast<uint32_t>(value);
  return stream.str();
}

// Logs an EGL error to stderr. This automatically calls eglGetError()
// and logs the error code.
static void LogEglError(std::string message) {
  EGLint error = eglGetError();
  FML_LOG(ERROR) << "EGL:" << message
                 << ", EGL: eglGetError returned: " << error;
}

static bool IsDeviceLostError(HRESULT hr) {
  switch (hr) {
    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DEVICE_RESET:
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
      return true;
    default:
      return false;
  }
}

static bool IsRetriableD3D11CreateError(HRESULT hr) {
  return IsDeviceLostError(hr) || hr == E_OUTOFMEMORY;
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
      egl_device_(nullptr) {}

HeadlessAngleSurfaceManager::~HeadlessAngleSurfaceManager() { CleanUp(); }

bool HeadlessAngleSurfaceManager::MakeCurrent() {
  if (!EnsureInitialized()) {
    return false;
  }
  return (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_,
                         egl_context_) == EGL_TRUE);
}

bool HeadlessAngleSurfaceManager::ClearContext() {
  if (!initialize_succeeded_) {
    return true;
  }
  return (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT) == EGL_TRUE);
}

Microsoft::WRL::ComPtr<ID3D11Device> HeadlessAngleSurfaceManager::GetDevice() {
  if (!EnsureInitialized()) {
    return nullptr;
  }
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
  auto create_device = [&]() {
    resolved_device_.Reset();
    return CreateSafeD3D11Device(D3D11CreateDevice, &resolved_device_, nullptr);
  };

  HRESULT hr = create_device();
  for (size_t retry = 0; FAILED(hr) && IsRetriableD3D11CreateError(hr) &&
                         retry < kDefaultD3D11DeviceRetryDelaysMs.size();
       ++retry) {
    FML_LOG(WARNING)
        << "HeadlessAngleSurfaceManager::TryInitializeD3D11Device, transient "
           "error while creating D3D11 Device, retry "
        << (retry + 1) << "/" << kDefaultD3D11DeviceRetryDelaysMs.size()
        << " after " << kDefaultD3D11DeviceRetryDelaysMs[retry]
        << "ms, hr:" << hr;
    ::Sleep(kDefaultD3D11DeviceRetryDelaysMs[retry]);
    hr = create_device();
  }

  if (FAILED(hr)) {
    FML_LOG(WARNING)
        << "HeadlessAngleSurfaceManager::TryInitializeD3D11Device, Could not "
           "create D3D11 Device, hr:"
        << hr << ", device_lost:" << IsDeviceLostError(hr);
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

bool HeadlessAngleSurfaceManager::EnsureInitialized() {
  if (initialize_attempted_) {
    return initialize_succeeded_;
  }

  initialize_attempted_ = true;
  initialize_succeeded_ = Initialize();
  if (!initialize_succeeded_) {
    CleanUp();
  }
  return initialize_succeeded_;
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
                                      EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
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
  bool initialized = false;
  for (size_t attempt = 0;
       !initialized && attempt <= kDefaultAngleInitRetryDelaysMs.size();
       ++attempt) {
    if (attempt > 0) {
      const DWORD retry_delay = kDefaultAngleInitRetryDelaysMs[attempt - 1];
      FML_LOG(INFO)
          << "HeadlessAngleSurfaceManager::Initialize, retrying ANGLE display "
             "configs, retry "
          << attempt << "/" << kDefaultAngleInitRetryDelaysMs.size()
          << " after " << retry_delay << "ms.";
      ::Sleep(retry_delay);
    }

    for (auto config : display_attributes_configs) {
      bool should_log = (attempt == kDefaultAngleInitRetryDelaysMs.size() &&
                         config == display_attributes_configs.back());
      if (InitializeEGL(egl_get_platform_display_EXT, config, should_log)) {
        initialized = true;
        break;
      }
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

  // The pbuffer is only used as the draw/read surface when making the context
  // current. Actual rendering targets a SharedImage-backed FBO with its own
  // depth/stencil attachment.
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

  if (resolved_device_) {
    FML_LOG(INFO)
        << "HeadlessAngleSurfaceManager::CleanUp before releasing D3D11 device"
        << ", surface_manager=" << this << ", egl_display=" << egl_display_
        << ", egl_context=" << egl_context_ << ", egl_surface=" << egl_surface_
        << ", d3d11_device=" << resolved_device_.Get()
        << ", device_removed_reason="
        << ToHexString(resolved_device_->GetDeviceRemovedReason());
  } else if (egl_display_ != EGL_NO_DISPLAY || egl_context_ != EGL_NO_CONTEXT ||
             egl_surface_ != EGL_NO_SURFACE) {
    FML_LOG(INFO)
        << "HeadlessAngleSurfaceManager::CleanUp before releasing D3D11 device"
        << ", surface_manager=" << this << ", egl_display=" << egl_display_
        << ", egl_context=" << egl_context_ << ", egl_surface=" << egl_surface_
        << ", d3d11_device=null";
  }

  // Needs to be reset before destroying the EGLContext.
  resolved_device_.Reset();

  if (egl_display_ != EGL_NO_DISPLAY && egl_surface_ != EGL_NO_SURFACE) {
    result = eglDestroySurface(egl_display_, egl_surface_);
    egl_surface_ = EGL_NO_SURFACE;
    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy surface");
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

GLFBOInfo ClayHeadlessRendererAngle::GLContextFBO(
    GLFrameInfo frame_info) const {
  GLFBOInfo info = ClayHeadlessRendererSharedImageGL::GLContextFBO(frame_info);
  info.has_depth_stencil_attachment = info.fbo_id >= 0;
  return info;
}

bool ClayHeadlessRendererAngle::MakeCurrent() {
  return surface_manager_->MakeCurrent();
}

bool ClayHeadlessRendererAngle::ClearCurrent() {
  return surface_manager_->ClearContext();
}

int64_t ClayHeadlessRendererAngle::FBO(const ClayFrameInfo& frame_info) {
  const int64_t fbo_id = ClayHeadlessRendererSharedImageGL::FBO(frame_info);
  if (fbo_id < 0) {
    return fbo_id;
  }

  if (attachment_width_ != frame_info.width ||
      attachment_height_ != frame_info.height) {
    ClearDepthStencilAttachments();
    attachment_width_ = frame_info.width;
    attachment_height_ = frame_info.height;
  }

  if (depth_stencil_attachments_.find(fbo_id) !=
      depth_stencil_attachments_.end()) {
    return fbo_id;
  }

  while (glGetError() != GL_NO_ERROR) {
  }

  GLint previous_fbo = 0;
  GLint previous_renderbuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);
  glGetIntegerv(GL_RENDERBUFFER_BINDING, &previous_renderbuffer);

  glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(fbo_id));
  GLuint depth_stencil_id = 0;
  glGenRenderbuffers(1, &depth_stencil_id);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_stencil_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                        frame_info.width, frame_info.height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_stencil_id);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, depth_stencil_id);

  const GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  const GLenum gl_error = glGetError();

  glBindRenderbuffer(GL_RENDERBUFFER, previous_renderbuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, previous_fbo);

  if (depth_stencil_id == 0 || gl_error != GL_NO_ERROR ||
      framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
    FML_LOG(ERROR) << "Failed to attach the headless depth/stencil buffer, "
                   << "GL error: " << gl_error
                   << ", framebuffer status: " << framebuffer_status;
    if (depth_stencil_id != 0) {
      glDeleteRenderbuffers(1, &depth_stencil_id);
    }
    return -1;
  }

  depth_stencil_attachments_.emplace(fbo_id, depth_stencil_id);
  return fbo_id;
}

void ClayHeadlessRendererAngle::CleanupGPUResources() {
  ClayHeadlessRendererSharedImageGL::CleanupGPUResources();
  ClearDepthStencilAttachments();
}

void ClayHeadlessRendererAngle::ClearDepthStencilAttachments() {
  for (const auto& attachment : depth_stencil_attachments_) {
    glDeleteRenderbuffers(1, &attachment.second);
  }
  depth_stencil_attachments_.clear();
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererAngle>(engine, config);
}

}  // namespace clay
