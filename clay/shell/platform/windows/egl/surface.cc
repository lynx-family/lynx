// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/surface.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include "clay/shell/platform/windows/egl/egl.h"

namespace {

bool HasExtension(const char* extensions, const char* name) {
  if (!extensions || !name || name[0] == '\0') return false;
  const char* start = extensions;
  while ((start = std::strstr(start, name)) != nullptr) {
    const char* end = start + std::strlen(name);
    if ((start == extensions || start[-1] == ' ') &&
        (*end == '\0' || *end == ' ')) {
      return true;
    }
    start = end;
  }
  return false;
}

std::array<EGLint, 4> DamageRectToEGLRect(const clay::Rect& rect,
                                          EGLint surface_height) {
  return std::array<EGLint, 4>{
      static_cast<EGLint>(rect.x()),
      surface_height - static_cast<EGLint>(rect.MaxY()),
      static_cast<EGLint>(rect.width()), static_cast<EGLint>(rect.height())};
}

clay::Rect ClipDamageRect(const clay::Rect& rect,
                          const clay::Rect& surface_rect) {
  if (rect.IsEmpty() || surface_rect.IsEmpty()) {
    return {};
  }

  // Compute the edges in 64 bits so malformed input cannot overflow while it
  // is being clipped to the EGL surface bounds.
  const std::int64_t left = std::max<std::int64_t>(rect.x(), surface_rect.x());
  const std::int64_t top = std::max<std::int64_t>(rect.y(), surface_rect.y());
  const std::int64_t right = std::min<std::int64_t>(
      static_cast<std::int64_t>(rect.x()) + rect.width(),
      static_cast<std::int64_t>(surface_rect.x()) + surface_rect.width());
  const std::int64_t bottom = std::min<std::int64_t>(
      static_cast<std::int64_t>(rect.y()) + rect.height(),
      static_cast<std::int64_t>(surface_rect.y()) + surface_rect.height());
  if (left >= right || top >= bottom) {
    return {};
  }

  return clay::Rect(static_cast<int>(left), static_cast<int>(top),
                    static_cast<int>(right - left),
                    static_cast<int>(bottom - top));
}

std::optional<clay::Rect> SurfaceRect(EGLDisplay display, EGLSurface surface) {
  EGLint width = 0;
  EGLint height = 0;
  if (eglQuerySurface(display, surface, EGL_WIDTH, &width) != EGL_TRUE ||
      eglQuerySurface(display, surface, EGL_HEIGHT, &height) != EGL_TRUE) {
    clay::egl::LogEGLError("Failed to query EGL surface size");
    return std::nullopt;
  }
  return clay::Rect(0, 0, width, height);
}

}  // namespace

namespace clay {
namespace egl {

// The number of buffers in the swap chain can range from 2 to 16. see DXGI Flip
// Model.
static constexpr int kMaxBufferCount = 16;

Surface::Surface(EGLDisplay display, EGLContext context, EGLSurface surface)
    : display_(display), context_(context), surface_(surface) {
  buffers_.resize(kMaxBufferCount);

  const char* extensions = eglQueryString(display_, EGL_EXTENSIONS);
  supports_buffer_age_ = HasExtension(extensions, "EGL_EXT_buffer_age");
  if (HasExtension(extensions, "EGL_EXT_swap_buffers_with_damage")) {
    swap_buffers_with_damage_ =
        reinterpret_cast<PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC>(
            eglGetProcAddress("eglSwapBuffersWithDamageEXT"));
  } else if (HasExtension(extensions, "EGL_KHR_swap_buffers_with_damage")) {
    swap_buffers_with_damage_ =
        reinterpret_cast<PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC>(
            eglGetProcAddress("eglSwapBuffersWithDamageKHR"));
  }
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
  EGLBoolean result = EGL_FALSE;
  std::optional<clay::Rect> present_damage = pending_present_damage_region_;
  const bool supports_swap_with_damage = SupportsSwapBuffersWithDamage();
  const bool tracks_buffer_age = SupportsBufferAge();
  std::optional<clay::Rect> surface_rect;
  if (supports_swap_with_damage || tracks_buffer_age) {
    surface_rect = SurfaceRect(display_, GetHandle());
    if (surface_rect.has_value()) {
      if (present_damage.has_value()) {
        present_damage = ClipDamageRect(*present_damage, *surface_rect);
      } else {
        present_damage = surface_rect;
      }
    } else {
      // A full swap is still safe, but without the surface size an existing
      // damage history cannot be updated reliably.
      present_damage.reset();
    }
  }

  bool attempted_swap_with_damage = false;
  if (supports_swap_with_damage && surface_rect.has_value() &&
      present_damage.has_value()) {
    attempted_swap_with_damage = true;
    if (present_damage->IsEmpty()) {
      result = swap_buffers_with_damage_(display_, GetHandle(), nullptr, 0);
    } else {
      const auto rect =
          DamageRectToEGLRect(*present_damage, surface_rect->height());
      result = swap_buffers_with_damage_(display_, GetHandle(), rect.data(), 1);
    }
  } else {
    result = ::eglSwapBuffers(display_, GetHandle());
  }

  if (attempted_swap_with_damage && result != EGL_TRUE) {
    LogEGLError("eglSwapBuffersWithDamage failed; falling back to a full swap");
    result = ::eglSwapBuffers(display_, GetHandle());
    if (result == EGL_TRUE) {
      present_damage = surface_rect;
    }
  }

  if (result != EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return false;
  }

  if (present_damage.has_value()) {
    AddDamageRegion(*present_damage);
  } else if (tracks_buffer_age) {
    damage_history_.clear();
  }
  pending_present_damage_region_.reset();
  return true;
}

const EGLSurface& Surface::GetHandle() const { return surface_; }

void Surface::SetDamageRegion(const clay::Rect&) {}

void Surface::SetPresentDamageRegion(const clay::Rect& region) {
  pending_present_damage_region_ = region;
}

std::optional<clay::Rect> Surface::GetDamageRegion() const {
  if (!SupportsBufferAge()) {
    return std::nullopt;
  }

  EGLint age = 0;
  if (eglQuerySurface(display_, GetHandle(), EGL_BUFFER_AGE_EXT, &age) !=
      EGL_TRUE) {
    WINDOWS_LOG_EGL_ERROR;
    return std::nullopt;
  }

  if (age == 0) {
    return std::nullopt;
  }

  size_t previous_frame_count = static_cast<size_t>(age - 1);
  if (previous_frame_count > damage_history_.size()) {
    return std::nullopt;
  }

  clay::Rect res = {};
  for (size_t i = 0; i < previous_frame_count; ++i) {
    res.Unite(damage_history_[damage_history_.size() - 1 - i]);
  }
  return res;
}

bool Surface::SupportsBufferAge() const { return supports_buffer_age_; }

bool Surface::SupportsSwapBuffersWithDamage() const {
  return swap_buffers_with_damage_ != nullptr;
}

void Surface::AddDamageRegion(const clay::Rect& region) {
  if (!SupportsBufferAge()) {
    return;
  }

  damage_history_.push_back(region);
  if (damage_history_.size() > kMaxBufferCount) {
    damage_history_.erase(damage_history_.begin());
  }
}

void Surface::AddDamageRegionWithManualBufferRotation(
    const clay::Rect& region) {
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

bool Surface::Resize(int width, int height) {
  buffers_.clear();
  buffers_.resize(buffer_count());
  damage_history_.clear();
  current_buffer_index_ = 0;
  pending_present_damage_region_.reset();
  return true;
}

}  // namespace egl
}  // namespace clay
