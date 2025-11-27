// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/surface.h"

#include "clay/shell/platform/windows/egl/egl.h"

namespace clay {
namespace egl {

// The number of buffers in the swap chain can range from 2 to 16. see DXGI Flip
// Model.
static constexpr int kMaxBufferCount = 16;

Surface::Surface(EGLDisplay display, EGLContext context, EGLSurface surface)
    : display_(display), context_(context), surface_(surface) {
  buffers_.resize(kMaxBufferCount);
}

Surface::~Surface() { Destroy(); }

bool Surface::IsValid() const { return is_valid_; }

bool Surface::Destroy() {
  if (surface_ != EGL_NO_SURFACE) {
    // Ensure the surface is not current before destroying it.
    if (::eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT) != EGL_TRUE) {
      WINDOWS_LOG_EGL_ERROR;
      return false;
    }

    if (::eglDestroySurface(display_, surface_) != EGL_TRUE) {
      WINDOWS_LOG_EGL_ERROR;
      return false;
    }
  }

  is_valid_ = false;
  surface_ = EGL_NO_SURFACE;
  return true;
}

bool Surface::IsCurrent() const {
  return display_ == ::eglGetCurrentDisplay() &&
         GetHandle() == ::eglGetCurrentSurface(EGL_DRAW) &&
         GetHandle() == ::eglGetCurrentSurface(EGL_READ) &&
         context_ == ::eglGetCurrentContext();
}

bool Surface::MakeCurrent() {
  if (::eglMakeCurrent(display_, GetHandle(), GetHandle(), context_) !=
      EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }

  return true;
}

bool Surface::SwapBuffers() {
  if (::eglSwapBuffers(display_, GetHandle()) != EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }
  return true;
}

const EGLSurface& Surface::GetHandle() const { return surface_; }

void Surface::SetDamageRegion(const clay::Rect& region) {}

std::optional<clay::Rect> Surface::GetDamageRegion() const {
  int age = buffers_[current_buffer_index_].age;
  if (age == 0) {
    return std::nullopt;
  } else {
    --age;
    clay::Rect res = {};
    for (size_t i = 0; i < buffer_count(); i++) {
      if (buffers_[i].age <= age) {
        res.Unite(buffers_[i].damage);
      }
    }
    return res;
  }
}

void Surface::AddDamageRegion(const clay::Rect& region) {
  buffers_[current_buffer_index_].damage = region;
  for (size_t i = 0; i < buffer_count(); ++i) {
    if (i == current_buffer_index_) {
      buffers_[i].age = 1;
    } else if (buffers_[i].age > 0) {
      ++buffers_[i].age;
    }
  }
  current_buffer_index_ = (current_buffer_index_ + 1) % buffer_count();
}

}  // namespace egl
}  // namespace clay
