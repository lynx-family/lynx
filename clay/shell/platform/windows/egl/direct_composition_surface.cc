// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/direct_composition_surface.h"

#include "clay/gfx/geometry/rect.h"
#include "clay/shell/platform/windows/egl/egl.h"

namespace {

RECT ToRECT(const clay::Rect& rect) {
  RECT r;
  r.left = rect.x();
  r.right = rect.MaxX();
  r.top = rect.y();
  r.bottom = rect.MaxY();
  return r;
}

}  // namespace

namespace clay {
namespace egl {

// If damage_rect / full_chrome_rect >= kForceFullDamageThreshold, present
// the swap chain with full damage.
static constexpr float kForceFullDamageThreshold = 0.6f;

DirectCompositionSurface::DirectCompositionSurface(
    EGLDisplay display, EGLContext context, EGLConfig config,
    EGLNativeWindowType window,
    Microsoft::WRL::ComPtr<IDCompositionDevice2> device, int width, int height,
    bool force_full_damage, bool force_full_damage_always)
    : WindowSurface(display, context, config, window, width, height),
      dcomp_device_(device),
      force_full_damage_(force_full_damage),
      force_full_damage_always_(force_full_damage_always) {
  d3d11_device_ = QueryD3D11DeviceObjectFromANGLE(display);
}

DirectCompositionSurface::~DirectCompositionSurface() { Destroy(); }

bool DirectCompositionSurface::Initialize() {
  if (display_ == EGL_NO_DISPLAY) {
    FML_LOG(ERROR) << "Trying to create surface with invalid display.";
    return false;
  }

  child_window_ = std::make_unique<ChildWindowWin>(window_);

  if (!child_window_->window()) {
    FML_LOG(ERROR) << "Trying to Initial child window failed.";
    return false;
  }

  FML_DCHECK(dcomp_device_);

  HRESULT hr;
  Microsoft::WRL::ComPtr<IDCompositionDesktopDevice> desktop_device;
  dcomp_device_.As(&desktop_device);
  FML_DCHECK(desktop_device);

  hr = desktop_device->CreateTargetForHwnd(
      static_cast<HWND>(child_window_->window()), TRUE, &dcomp_target_);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "CreateTargetForHwnd failed with error 0x" << std::hex
                   << hr;
    return false;
  }
  dcomp_device_->CreateVisual(&dcomp_root_visual_);
  FML_DCHECK(dcomp_root_visual_);
  dcomp_target_->SetRoot(dcomp_root_visual_.Get());
  // A visual inherits the interpolation mode of the parent visual by default.
  // If no visuals set the interpolation mode, the default for the entire visual
  // tree is nearest neighbor interpolation.
  // Set the interpolation mode to Linear to get a better upscaling quality.
  dcomp_root_visual_->SetBitmapInterpolationMode(
      DCOMPOSITION_BITMAP_INTERPOLATION_MODE_LINEAR);
  EGLint pbuffer_attribs[] = {
      EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
  };

  default_surface_ =
      eglCreatePbufferSurface(display_, config_, pbuffer_attribs);
  if (!default_surface_) {
    FML_LOG(ERROR) << "eglCreatePbufferSurface failed with error ";
    return false;
  }

  is_valid_ = true;
  return true;
}

bool DirectCompositionSurface::Destroy() {
  // Ensure the surface is not current before destroying it.
  if (::eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT) != EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }
  if (default_surface_) {
    if (!eglDestroySurface(display_, default_surface_)) {
      FML_LOG(ERROR) << "eglDestroySurface failed with error";
    }
    default_surface_ = nullptr;
  }
  if (real_surface_) {
    if (!eglDestroySurface(display_, real_surface_)) {
      FML_LOG(ERROR) << "eglDestroySurface failed with error";
    }
    real_surface_ = nullptr;
  }
  if (dcomp_target_) {
    dcomp_target_->SetRoot(nullptr);
    dcomp_target_.Reset();
  }
  if (dcomp_root_visual_) {
    dcomp_root_visual_->RemoveAllVisuals();
    dcomp_root_visual_.Reset();
  }
  draw_texture_.Reset();

  is_valid_ = false;
  return true;
}

bool DirectCompositionSurface::MakeCurrent() {
  if (::eglMakeCurrent(display_, GetHandle(), GetHandle(), context_) !=
      EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }
  return true;
}

bool DirectCompositionSurface::SwapBuffers() {
  bool swap_result = ReleaseDrawTexture(false);
  AddDamageRegion(swap_rect_);
  swap_rect_.Clear();
  return swap_result;
}

bool DirectCompositionSurface::Resize(int width, int height) {
  child_window_->Resize(width, height);
  if (!Surface::Destroy()) {
    FML_LOG(ERROR) << "Surface resize failed to destroy surface";
    return false;
  }
  is_valid_ = true;
  draw_texture_.Reset();
  size_ = {width, height};
  // This will release indirect references to swap chain (|real_surface_|) by
  // binding |default_surface_| as the default framebuffer.
  if (!ReleaseDrawTexture(true /* will_discard */)) return false;

  // ResizeBuffers can't change alpha blending mode.
  if (swap_chain_) {
    UINT count = buffer_count();
    DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
    UINT flags =
        IsSwapChainTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    d3d11_device_->GetImmediateContext(&context);
    HRESULT hr =
        swap_chain_->ResizeBuffers(count, width, height, format, flags);
    if (!SUCCEEDED(hr)) {
      FML_DLOG(ERROR) << "ResizeBuffers failed with error 0x" << std::hex << hr;
      return false;
    }
    if (!MakeCurrent() || !SetVSyncEnabled(vsync_enabled())) {
      // Surfaces block until the v-blank by default.
      // Failing to update the vsync might result in unnecessary blocking.
      // This regresses performance but not correctness.
      FML_LOG(ERROR) << "Surface resize failed to set vsync";
    }
    return true;
  }
  // Next SetDrawRectangle call will recreate the swap chain or surface.
  swap_chain_.Reset();
  return false;
}

const EGLSurface& DirectCompositionSurface::GetHandle() const {
  return real_surface_ ? real_surface_ : default_surface_;
}

void DirectCompositionSurface::SetDamageRegion(const clay::Rect& region) {
  SetDrawRectangle(region);
  if (::eglMakeCurrent(display_, GetHandle(), GetHandle(), context_) !=
      EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
  }
}

bool DirectCompositionSurface::ReleaseDrawTexture(bool will_discard) {
  EGLSurface egl_surface = real_surface_;
  real_surface_ = nullptr;
  if (egl_surface) {
    eglDestroySurface(display_, egl_surface);
    egl_surface = nullptr;
  }

  // If MakeCurrent fails (probably lost device), we'll want to return failure,
  // but we still want to reset the rest of the state for consistency.
  if (::eglMakeCurrent(display_, GetHandle(), GetHandle(), context_) !=
      EGL_TRUE) {
    LogEGLError("Failed to make current in ReleaseDrawTexture");
    return false;
  }

  HRESULT hr, device_removed_reason;
  if (draw_texture_) {
    draw_texture_.Reset();

    if (!will_discard) {
      const bool use_swap_chain_tearing = IsSwapChainTearingSupported();
      UINT interval =
          first_swap_ || !vsync_enabled() || use_swap_chain_tearing ? 0 : 1;
      UINT flags = use_swap_chain_tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

      bool actually_force_full_damage = false;
      if (force_full_damage_) {
        if (force_full_damage_always_) {
          actually_force_full_damage = true;
        } else {
          float percentage = swap_rect_.size().GetArea();
          percentage /= size_.GetArea();
          if (percentage >= kForceFullDamageThreshold)
            actually_force_full_damage = true;
        }
      }
      if (actually_force_full_damage) {
        hr = swap_chain_->Present(interval, flags);
      } else {
        DXGI_PRESENT_PARAMETERS params = {};
        RECT dirty_rect = ToRECT(swap_rect_);
        params.DirtyRectsCount = 1;
        params.pDirtyRects = &dirty_rect;

        hr = swap_chain_->Present1(interval, flags, &params);
      }
      if (FAILED(hr) && hr != DXGI_STATUS_OCCLUDED) {
        FML_LOG(ERROR) << "swap_chain_->Present failed. "
                       << "hr=0x" << std::hex << hr << ", interval=" << interval
                       << ", flags=0x" << std::hex << flags;
        return false;
      }
      // Wait for the GPU to finish executing its commands before
      // committing the DirectComposition tree, or else the swapchain
      // may flicker black when it's first presented.
      if (first_swap_) {
        first_swap_ = false;
        Microsoft::WRL::ComPtr<IDXGIDevice2> dxgi_device2;
        hr = d3d11_device_.As(&dxgi_device2);
        if (SUCCEEDED(hr) && dxgi_device2) {
          HANDLE event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
          if (event_handle) {
            hr = dxgi_device2->EnqueueSetEvent(event_handle);
            if (SUCCEEDED(hr)) {
              WaitForSingleObject(event_handle, INFINITE);
            } else {
              HRESULT device_removed_reason =
                  d3d11_device_->GetDeviceRemovedReason();
              FML_LOG(ERROR) << "EnqueueSetEvent failed. hr=0x" << std::hex
                             << hr << ", device_removed_reason=0x" << std::hex
                             << device_removed_reason;
            }
            CloseHandle(event_handle);
          }
        }
      }
      hr = dcomp_device_->Commit();
      if (FAILED(hr)) {
        FML_LOG(ERROR) << "dcomp_device_->Commit() failed. hr=0x" << std::hex
                       << hr;
        return false;
      }
    }
  }
  return true;
}

bool DirectCompositionSurface::SetDrawRectangle(const clay::Rect& rectangle) {
  auto rect = clay::Rect(size_);
  if (!rect.Contains(rectangle)) {
    FML_DLOG(ERROR)
        << "Draw rectangle must be contained within size of surface";
    return false;
  }

  if (draw_texture_) {
    FML_DLOG(ERROR)
        << "SetDrawRectangle must be called only once per swap buffers";
    return false;
  }

  if (rect != rectangle && !swap_chain_) {
    FML_DLOG(ERROR) << "First draw to surface must draw to everything";
    return false;
  }

  DXGI_FORMAT dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;

  if (!swap_chain_ &&
      ((!false || dxgi_format == DXGI_FORMAT_R10G10B10A2_UNORM))) {
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    d3d11_device_.As(&dxgi_device);
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter;
    dxgi_device->GetAdapter(&dxgi_adapter);
    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory;
    dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width();
    desc.Height = height();
    desc.Format = dxgi_format;
    desc.SampleDesc.Count = 1;
    desc.BufferCount = buffer_count();
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode =
        has_alpha_ ? DXGI_ALPHA_MODE_PREMULTIPLIED : DXGI_ALPHA_MODE_IGNORE;
    desc.Flags =
        IsSwapChainTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    HRESULT hr = dxgi_factory->CreateSwapChainForComposition(
        d3d11_device_.Get(), &desc, nullptr, &swap_chain_);
    first_swap_ = true;
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "CreateSwapChainForComposition failed. "
                     << "hr=0x" << std::hex << hr
                     << ", d3d11_device_=" << d3d11_device_.Get()
                     << ", swap_chain_=" << swap_chain_.Get();
      return false;
    }
    dcomp_root_visual_->SetContent(swap_chain_.Get());
  }
  swap_rect_ = rectangle;

  swap_chain_->GetBuffer(0, IID_PPV_ARGS(&draw_texture_));

  FML_DCHECK(draw_texture_);

  std::vector<EGLint> pbuffer_attribs = {
      EGL_WIDTH, static_cast<EGLint>(width()), EGL_HEIGHT,
      static_cast<EGLint>(height()), EGL_NONE};

  EGLClientBuffer buffer =
      reinterpret_cast<EGLClientBuffer>(draw_texture_.Get());
  real_surface_ = eglCreatePbufferFromClientBuffer(
      display_, EGL_D3D_TEXTURE_ANGLE, buffer, config_, pbuffer_attribs.data());

  if (!real_surface_) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }

  return true;
}

bool DirectCompositionSurface::IsSwapChainTearingSupported() {
  static const bool supported = [=] {
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device = d3d11_device_;
    if (!d3d11_device) {
      return false;
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = d3d11_device.As(&dxgi_device);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to query IDXGIDevice from d3d11_device. hr=0x"
                     << std::hex << hr;
      return false;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter;
    hr = dxgi_device->GetAdapter(&dxgi_adapter);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to get IDXGIAdapter from IDXGIDevice. hr=0x"
                     << std::hex << hr;
      return false;
    }

    Microsoft::WRL::ComPtr<IDXGIFactory5> dxgi_factory;
    hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to get IDXGIFactory5 from IDXGIAdapter. hr=0x"
                     << std::hex << hr;
      return false;
    }

    BOOL present_allow_tearing = FALSE;
    hr = dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                           &present_allow_tearing,
                                           sizeof(present_allow_tearing));
    if (FAILED(hr)) {
      FML_LOG(ERROR)
          << "Failed to check DXGI_FEATURE_PRESENT_ALLOW_TEARING. hr=0x"
          << std::hex << hr;
      return false;
    }

    return present_allow_tearing == TRUE;
  }();

  return supported;
}

Microsoft::WRL::ComPtr<ID3D11Device>
DirectCompositionSurface::QueryD3D11DeviceObjectFromANGLE(
    EGLDisplay egl_display) {
  if (egl_display == EGL_NO_DISPLAY) {
    LogEGLError("Failed to retrieve EGLDisplay");
    return nullptr;
  }

  intptr_t egl_device = 0;
  if (!eglQueryDisplayAttribEXT(egl_display, EGL_DEVICE_EXT, &egl_device)) {
    LogEGLError("eglQueryDisplayAttribEXT failed");
    return nullptr;
  }

  if (!egl_device) {
    LogEGLError("Failed to retrieve EGLDeviceEXT");
    return nullptr;
  }
  intptr_t device = 0;
  if (!eglQueryDeviceAttribEXT(reinterpret_cast<EGLDeviceEXT>(egl_device),
                               EGL_D3D11_DEVICE_ANGLE, &device)) {
    LogEGLError("eglQueryDeviceAttribEXT failed");
    return nullptr;
  }
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
  d3d11_device = reinterpret_cast<ID3D11Device*>(device);
  return d3d11_device;
}

}  // namespace egl
}  // namespace clay
