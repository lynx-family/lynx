// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_SURFACE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_SURFACE_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <optional>
#include <vector>

#include "base/include/fml/macros.h"
#include "clay/gfx/geometry/rect.h"

namespace clay {
namespace egl {

enum class GLImplementationType {
  kAngleEGL,
  kAngleEGLDirectComposition,
};

struct BufferInfo {
  uint32_t age = 0;
  clay::Rect damage;
};

// An EGL surface. This can be window surface or an off-screen buffer.
//
// This enables automatic error logging and mocking.
class Surface {
 public:
  Surface(EGLDisplay display, EGLContext context, EGLSurface surface);

  virtual ~Surface();

  // Destroy the EGL surface and invalidate this object.
  //
  // This also unbinds the current context from the thread.
  virtual bool Destroy();

  // Check whether the EGL surface is valid.
  virtual bool IsValid() const;

  // Check whether the EGL display, context, and surface are bound
  // to the current thread.
  virtual bool IsCurrent() const;

  // Bind the EGL surface's context and read and draw surfaces to the
  // current thread. Returns true on success.
  virtual bool MakeCurrent();

  // Swap the surface's front the and back buffers. Used to present content.
  // Returns true on success.
  virtual bool SwapBuffers();

  // Resizes the underlying surface or resource to the specified width and
  // height.
  virtual bool Resize(int width, int height);

  virtual uint32_t buffer_count() const { return 1; }

  // Get the raw EGL surface.
  virtual const EGLSurface& GetHandle() const;

  // Buffer damage used before rendering. The base EGL surface does not
  // implement EGL_KHR_partial_update; DirectComposition overrides this to open
  // the draw surface for the current backbuffer.
  virtual void SetDamageRegion(const clay::Rect& region);

  // Frame damage used for present dirty rects and buffer-age history.
  virtual void SetPresentDamageRegion(const clay::Rect& region);

  virtual std::optional<clay::Rect> GetDamageRegion() const;

  virtual bool RequiresYAxisFlip() const { return false; }

 protected:
  void AddDamageRegion(const clay::Rect& rect);
  // For surfaces that own their swapchain rotation instead of relying on EGL.
  void AddDamageRegionWithManualBufferRotation(const clay::Rect& rect);
  bool SupportsBufferAge() const;
  bool SupportsSwapBuffersWithDamage() const;

  bool is_valid_ = true;

  EGLDisplay display_ = EGL_NO_DISPLAY;
  EGLContext context_ = EGL_NO_CONTEXT;
  EGLSurface surface_ = EGL_NO_SURFACE;

  std::vector<BufferInfo> buffers_;
  std::vector<clay::Rect> damage_history_;
  int current_buffer_index_ = 0;
  std::optional<clay::Rect> pending_present_damage_region_;
  bool supports_buffer_age_ = false;
  PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage_ = nullptr;

  BASE_DISALLOW_COPY_AND_ASSIGN(Surface);
};

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_SURFACE_H_
