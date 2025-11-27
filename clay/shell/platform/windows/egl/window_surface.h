// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_WINDOW_SURFACE_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_WINDOW_SURFACE_H_

#include <EGL/egl.h>

#include "base/include/fml/macros.h"
#include "clay/gfx/geometry/rect.h"
#include "clay/gfx/geometry/size.h"
#include "clay/shell/platform/windows/egl/surface.h"

namespace clay {
namespace egl {

// An EGL surface used to render a Flutter view to a win32 HWND.
//
// This enables automatic error logging and mocking.
class WindowSurface : public Surface {
 public:
  WindowSurface(EGLDisplay display, EGLContext context, EGLSurface surface,
                int width, int height);
  WindowSurface(EGLDisplay display, EGLContext context, EGLConfig config,
                EGLNativeWindowType window, int width, int height);

  virtual bool Initialize();
  // If enabled, makes the surface's buffer swaps block until the v-blank.
  //
  // If disabled, allows one thread to swap multiple buffers per v-blank
  // but can result in screen tearing if the system compositor is disabled.
  //
  // The surface must be current before calling this.
  virtual bool SetVSyncEnabled(bool enabled);

  // Get the surface's width in physical pixels.
  virtual int width() const;

  // Get the surface's height in physical pixels.
  virtual int height() const;

  // Get whether the surface's buffer swap blocks until the v-blank.
  virtual bool vsync_enabled() const;

  bool Resize(int width, int height) override;

 protected:
  clay::Size size_ = {0, 0};
  EGLNativeWindowType window_ = 0;
  EGLConfig config_;

 private:
  bool vsync_enabled_ = true;

  BASE_DISALLOW_COPY_AND_ASSIGN(WindowSurface);
};

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_WINDOW_SURFACE_H_
