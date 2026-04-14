// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/manager.h"

#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/shell/platform/windows/egl/direct_composition_surface.h"
#include "clay/shell/platform/windows/egl/egl.h"

namespace clay {
namespace egl {

int Manager::instance_count_ = 0;

std::unique_ptr<Manager> Manager::Create() {
  std::unique_ptr<Manager> manager;
  manager.reset(new Manager());
  if (!manager->IsValid()) {
    return nullptr;
  }
  return std::move(manager);
}

Manager::Manager() {
  ++instance_count_;

#ifndef CLAY_FORCE_D3D9
  if (!TryInitializeD3D11Device()) {
    return;
  }
#endif

  if (!InitializeDisplay()) {
    return;
  }

  if (!InitializeConfig()) {
    return;
  }

  if (!InitializeContexts()) {
    return;
  }

  is_valid_ = true;
}

Manager::~Manager() {
  CleanUp();
  --instance_count_;
}

bool Manager::InitializeDisplay() {
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
          ::eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (!egl_get_platform_display_EXT) {
    LogEGLError("eglGetPlatformDisplayEXT not available");
    return false;
  }

  // Attempt to initialize ANGLE's renderer in order of: D3D11, D3D11 Feature
  // Level 9_3 and finally D3D11 WARP.
  for (auto config : display_attributes_configs) {
    bool is_last = (config == display_attributes_configs.back());
    if (egl_device_ != nullptr) {
      display_ = egl_get_platform_display_EXT(EGL_PLATFORM_DEVICE_EXT,
                                              egl_device_, config);
    } else {
      display_ = egl_get_platform_display_EXT(EGL_PLATFORM_ANGLE_ANGLE,
                                              EGL_DEFAULT_DISPLAY, config);
    }

    if (display_ == EGL_NO_DISPLAY) {
      if (is_last) {
        LogEGLError("Failed to get a compatible EGLdisplay");
        return false;
      }

      // Try the next config.
      continue;
    }

    if (::eglInitialize(display_, nullptr, nullptr) == EGL_FALSE) {
      if (is_last) {
        LogEGLError("Failed to initialize EGL via ANGLE");
        return false;
      }

      // Try the next config.
      continue;
    }

    return true;
  }

  FML_UNREACHABLE();
}

bool Manager::InitializeConfig() {
  const EGLint config_attributes[] = {EGL_RED_SIZE,   8, EGL_GREEN_SIZE,   8,
                                      EGL_BLUE_SIZE,  8, EGL_ALPHA_SIZE,   8,
                                      EGL_DEPTH_SIZE, 8, EGL_STENCIL_SIZE, 8,
                                      EGL_NONE};

  EGLint num_config = 0;

  EGLBoolean result =
      ::eglChooseConfig(display_, config_attributes, &config_, 1, &num_config);

  if (result == EGL_TRUE && num_config > 0) {
    return true;
  }

  LogEGLError("Failed to choose EGL config");
  return false;
}

bool Manager::InitializeContexts() {
  const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  auto const render_context =
      ::eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attributes);
  if (render_context == EGL_NO_CONTEXT) {
    LogEGLError("Failed to create EGL render context");
    return false;
  }

  auto const resource_context =
      ::eglCreateContext(display_, config_, render_context, context_attributes);
  if (resource_context == EGL_NO_CONTEXT) {
    LogEGLError("Failed to create EGL resource context");
    return false;
  }

  render_context_ = std::make_unique<Context>(display_, render_context);
  resource_context_ = std::make_unique<Context>(display_, resource_context);
  return true;
}

bool Manager::InitializeDevice() {
  const auto query_display_attrib_EXT =
      reinterpret_cast<PFNEGLQUERYDISPLAYATTRIBEXTPROC>(
          ::eglGetProcAddress("eglQueryDisplayAttribEXT"));
  const auto query_device_attrib_EXT =
      reinterpret_cast<PFNEGLQUERYDEVICEATTRIBEXTPROC>(
          ::eglGetProcAddress("eglQueryDeviceAttribEXT"));

  if (query_display_attrib_EXT == nullptr ||
      query_device_attrib_EXT == nullptr) {
    return false;
  }

  EGLAttrib egl_device = 0;
  EGLAttrib angle_device = 0;

  auto result = query_display_attrib_EXT(display_, EGL_DEVICE_EXT, &egl_device);
  if (result != EGL_TRUE) {
    return false;
  }

  result = query_device_attrib_EXT(reinterpret_cast<EGLDeviceEXT>(egl_device),
                                   EGL_D3D11_DEVICE_ANGLE, &angle_device);
  if (result != EGL_TRUE) {
    return false;
  }

  resolved_device_ = reinterpret_cast<ID3D11Device*>(angle_device);
  return true;
}

bool Manager::TryInitializeD3D11Device() {
  // Note: Different raster threads cannot use the same egldisplay.
  d3d11_ = fml::NativeLibrary::Create("d3d11.dll");

  if (!d3d11_) {
    FML_LOG(WARNING) << "Could not load D3D11 library.";
    return false;
  }

  std::optional<PFN_D3D11_CREATE_DEVICE> D3D11CreateDevice =
      d3d11_->ResolveFunction<PFN_D3D11_CREATE_DEVICE>("D3D11CreateDevice");

  if (!D3D11CreateDevice.has_value()) {
    FML_LOG(WARNING) << "Could not retrieve D3D11CreateDevice address.";
    return false;
  }
  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1,
                                        D3D_FEATURE_LEVEL_11_0};
  HRESULT hr = D3D11CreateDevice.value()(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels,
      ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &resolved_device_, nullptr,
      nullptr);
  if (FAILED(hr)) {
    FML_LOG(WARNING) << "Could not create D3D11 Device.";
    return false;
  }

  egl_device_ = eglCreateDeviceANGLE(EGL_D3D11_DEVICE_ANGLE,
                                     resolved_device_.Get(), nullptr);
  if (!egl_device_) {
    FML_LOG(WARNING) << "Could not create EGL device.";
    return false;
  }

  return true;
}

bool Manager::TryInitializeDirectCompositionDevice() {
  if (dcomp_device_) {
    return true;
  }
  // Load DLL at runtime since older Windows versions don't have dcomp.
  if (!dcomp_) {
    dcomp_ = fml::NativeLibrary::Create("dcomp.dll");
  }

  if (!dcomp_) {
    FML_LOG(WARNING) << "Could not load DCOMP library.";
    return false;
  }
  using PFN_DCOMPOSITION_CREATE_DEVICE2 = HRESULT(WINAPI*)(
      IUnknown * renderingDevice, REFIID iid, void** dcompositionDevice);
  std::optional<PFN_DCOMPOSITION_CREATE_DEVICE2> create_device_function =
      dcomp_->ResolveFunction<PFN_DCOMPOSITION_CREATE_DEVICE2>(
          "DCompositionCreateDevice2");

  if (!create_device_function.has_value()) {
    FML_LOG(ERROR) << "GetProcAddress failed for DCompositionCreateDevice2";
    return false;
  }

  HRESULT hr;
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
  resolved_device_.As(&dxgi_device);

  Microsoft::WRL::ComPtr<IDCompositionDesktopDevice> desktop_device;
  hr = create_device_function.value()(dxgi_device.Get(),
                                      IID_PPV_ARGS(&desktop_device));
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "DCompositionCreateDevice2 failed with error 0x"
                   << std::hex << hr;
    return false;
  }

  hr = desktop_device.As(&dcomp_device_);

  if (FAILED(hr)) {
    FML_LOG(ERROR) << "IDCompositionDevice2 create failed with error 0x"
                   << std::hex << hr;
    return false;
  }
  return true;
}

void Manager::CleanUp() {
  EGLBoolean result = EGL_FALSE;

  // Needs to be reset before destroying the contexts.
  resolved_device_.Reset();

  // Needs to be reset before destroying the EGLDisplay.
  render_context_.reset();
  resource_context_.reset();

  if (display_ != EGL_NO_DISPLAY) {
    if (egl_device_ != nullptr) {
      // If the display is created from an owned device,
      // it needs to be released.
      eglTerminate(display_);
    }
    display_ = EGL_NO_DISPLAY;
  }

  if (egl_device_ != nullptr) {
    eglReleaseDeviceANGLE(egl_device_);
    egl_device_ = nullptr;
  }
}

bool Manager::IsValid() const { return is_valid_; }

std::unique_ptr<WindowSurface> Manager::CreateWindowSurface(
    GLImplementationType type, HWND hwnd, size_t width, size_t height) {
  if (!hwnd || !is_valid_) {
    return nullptr;
  }
  std::unique_ptr<WindowSurface> surface = nullptr;
  if (type == GLImplementationType::kAngleEGL) {
    surface = std::make_unique<WindowSurface>(
        display_, render_context_->GetHandle(), config_,
        static_cast<EGLNativeWindowType>(hwnd), width, height);
  } else if (type == GLImplementationType::kAngleEGLDirectComposition) {
    if (TryInitializeDirectCompositionDevice()) {
      surface = std::make_unique<DirectCompositionSurface>(
          display_, render_context_->GetHandle(), config_,
          static_cast<EGLNativeWindowType>(hwnd), dcomp_device_, width, height);
    } else {
      surface = std::make_unique<WindowSurface>(
          display_, render_context_->GetHandle(), config_,
          static_cast<EGLNativeWindowType>(hwnd), width, height);
    }
  } else {
    return nullptr;
  }
  if (!surface->Initialize()) {
    return nullptr;
  }
  return surface;
}

bool Manager::HasContextCurrent() {
  return ::eglGetCurrentContext() != EGL_NO_CONTEXT;
}

EGLSurface Manager::CreateSurfaceFromHandle(EGLenum handle_type,
                                            EGLClientBuffer handle,
                                            const EGLint* attributes) const {
  return ::eglCreatePbufferFromClientBuffer(display_, handle_type, handle,
                                            config_, attributes);
}

bool Manager::GetDevice(ID3D11Device** device) {
  if (!resolved_device_) {
    if (!InitializeDevice()) {
      return false;
    }
  }

  resolved_device_.CopyTo(device);
  return (resolved_device_ != nullptr);
}

Context* Manager::render_context() const { return render_context_.get(); }

Context* Manager::resource_context() const { return resource_context_.get(); }

}  // namespace egl
}  // namespace clay
