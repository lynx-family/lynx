// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/window_surface.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <utility>

#include "clay/fml/logging.h"
#include "clay/shell/platform/windows/egl/egl.h"

namespace clay {
namespace egl {

WindowSurface::WindowSurface(EGLDisplay display, EGLContext context,
                             EGLSurface surface, int width, int height)
    : Surface(display, context, surface), size_({width, height}) {}

WindowSurface::WindowSurface(EGLDisplay display, EGLContext context,
                             EGLConfig config, EGLNativeWindowType window,
                             int width, int height)
    : Surface(display, context, EGL_NO_SURFACE),
      size_({width, height}),
      config_(config),
      window_(window) {}

bool WindowSurface::Initialize() {
  if (display_ == EGL_NO_DISPLAY) {
    FML_LOG(ERROR) << "Trying to create surface with invalid display.";
    return false;
  }
  // Disable ANGLE's automatic surface resizing and provide an explicit size.
  // The surface will need to be destroyed and re-created if the HWND is
  // resized.
  const EGLint surface_attributes[] = {EGL_FIXED_SIZE_ANGLE,
                                       EGL_TRUE,
                                       EGL_WIDTH,
                                       static_cast<EGLint>(size_.width()),
                                       EGL_HEIGHT,
                                       static_cast<EGLint>(size_.height()),
                                       EGL_NONE};
  auto const surface =
      ::eglCreateWindowSurface(display_, config_, window_, surface_attributes);
  if (surface == EGL_NO_SURFACE) {
    FML_LOG(ERROR) << "Surface creation failed.";
    return false;
  }
  surface_ = std::move(surface);
  is_valid_ = true;
  return true;
}

bool WindowSurface::SetVSyncEnabled(bool enabled) {
  FML_DCHECK(IsCurrent());

  if (::eglSwapInterval(display_, enabled ? 1 : 0) != EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }

  vsync_enabled_ = enabled;
  return true;
}

int WindowSurface::width() const { return size_.width(); }

int WindowSurface::height() const { return size_.height(); }

bool WindowSurface::vsync_enabled() const { return vsync_enabled_; }

bool WindowSurface::Resize(int width, int height) {
  if (!Surface::Destroy()) {
    FML_LOG(ERROR) << "Surface resize failed to destroy surface";
    return false;
  }
  size_ = {width, height};
  if (!Initialize()) {
    FML_LOG(ERROR) << "Surface resize failed to create surface";
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

}  // namespace egl
}  // namespace clay
