
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_DIRECT_COMPOSITION_SURFACE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_DIRECT_COMPOSITION_SURFACE_H_

// OpenGL ES and EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <d3d11.h>
#include <dcomp.h>
#include <windows.h>
#include <wrl/client.h>

#include <memory>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/rect.h"
#include "clay/gfx/geometry/size.h"
#include "clay/shell/platform/windows/egl/child_window_win.h"
#include "clay/shell/platform/windows/egl/window_surface.h"

namespace clay {
namespace egl {

class DirectCompositionSurface : public WindowSurface {
 public:
  DirectCompositionSurface(EGLDisplay display, EGLContext context,
                           EGLConfig config, EGLNativeWindowType window,
                           Microsoft::WRL::ComPtr<IDCompositionDevice2> device,
                           int width, int height, bool force_full_damage,
                           bool force_full_damage_always);

  ~DirectCompositionSurface() override;

  bool Initialize() override;

  bool Destroy() override;

  bool MakeCurrent() override;

  bool SwapBuffers() override;

  bool Resize(int width, int height) override;

  const EGLSurface& GetHandle() const override;

  void SetDamageRegion(const clay::Rect& region) override;

  uint32_t buffer_count() const override { return 2; }

 private:
  bool SetDrawRectangle(const clay::Rect& rectangle);
  bool ReleaseDrawTexture(bool will_discard);
  Microsoft::WRL::ComPtr<ID3D11Device> QueryD3D11DeviceObjectFromANGLE(
      EGLDisplay egl_display);
  bool IsSwapChainTearingSupported();

  // This is a placeholder surface used when not rendering to the
  // DirectComposition surface.
  EGLSurface default_surface_ = 0;

  // This is the real surface representing the backbuffer. It may be null
  // outside of a BeginDraw/EndDraw pair.
  EGLSurface real_surface_ = 0;

  clay::Rect swap_rect_;
  bool has_alpha_ = true;
  bool first_swap_ = true;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  Microsoft::WRL::ComPtr<IDCompositionDevice2> dcomp_device_;
  Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> draw_texture_;
  Microsoft::WRL::ComPtr<IDCompositionVisual2> dcomp_root_visual_;
  Microsoft::WRL::ComPtr<IDCompositionTarget> dcomp_target_;

  bool force_full_damage_ = true;
  bool force_full_damage_always_ = false;

  std::unique_ptr<ChildWindowWin> child_window_;
};

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_DIRECT_COMPOSITION_SURFACE_H_
