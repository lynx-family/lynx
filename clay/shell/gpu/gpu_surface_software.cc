// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/gpu/gpu_surface_software.h"

#include <memory>

#include "clay/flow/surface_frame.h"
#include "clay/fml/logging.h"

namespace clay {

GPUSurfaceSoftware::GPUSurfaceSoftware(GPUSurfaceSoftwareDelegate* delegate,
                                       bool render_to_surface)
    : delegate_(delegate),
      render_to_surface_(render_to_surface),
      weak_factory_(this) {}

GPUSurfaceSoftware::~GPUSurfaceSoftware() = default;

// |Surface|
bool GPUSurfaceSoftware::IsValid() { return delegate_ != nullptr; }

// |Surface|
std::unique_ptr<SurfaceFrame> GPUSurfaceSoftware::AcquireFrame(
    const skity::Vec2& logical_size) {
  SurfaceFrame::FramebufferInfo framebuffer_info;
  framebuffer_info.supports_readback = true;

  // TODO(38466): Refactor GPU surface APIs take into account the fact that an
  // external view embedder may want to render to the root surface.
  if (!render_to_surface_) {
    return std::make_unique<SurfaceFrame>(
        nullptr, framebuffer_info,
        [](const SurfaceFrame& surface_frame, clay::GrCanvas* canvas) {
          return true;
        },
        [](const SurfaceFrame::SubmitInfo& surface_frame) { return true; },
        logical_size);
  }

  if (!IsValid()) {
    return nullptr;
  }
  const skity::Vec2 size = {logical_size.x, logical_size.y};

  clay::GrSurfacePtr backing_store = delegate_->AcquireBackingStore(size);

  if (backing_store == nullptr) {
    return nullptr;
  }

  if (size != skity::Vec2({SURFACE_GET_WIDTH(backing_store),
                           SURFACE_GET_HEIGHT(backing_store)})) {
    return nullptr;
  }

  // If the surface has been scaled, we need to apply the inverse scaling to the
  // underlying canvas so that coordinates are mapped to the same spot
  // irrespective of surface scaling.
  clay::GrCanvas* canvas = SURFACE_GET_CANVAS(backing_store, false);
  CANVAS_RESET_MATRIX(canvas);

  SurfaceFrame::EncodeCallback encode_callback =
      [self = weak_factory_.GetWeakPtr()](const SurfaceFrame& surface_frame,
                                          clay::GrCanvas* canvas) -> bool {
    // If the surface itself went away, there is nothing more to do.
    if (!self || !self->IsValid() || canvas == nullptr) {
      return false;
    }

    CANVAS_FLUSH(canvas);
    return self->delegate_->PresentBackingStore(surface_frame.GetSurface());
  };
  SurfaceFrame::SubmitCallback submit_callback =
      [self = weak_factory_.GetWeakPtr()](
          const SurfaceFrame::SubmitInfo& surface_frame) { return true; };

  return std::make_unique<SurfaceFrame>(backing_store, framebuffer_info,
                                        encode_callback, submit_callback,
                                        logical_size);
}

// |Surface|
skity::Matrix GPUSurfaceSoftware::GetRootTransformation() const {
  // This backend does not currently support root surface transformations. Just
  // return identity.
  return skity::Matrix();
}

// |Surface|
clay::GrContext* GPUSurfaceSoftware::GetContext() {
  // There is no GrContext associated with a software surface.
  return nullptr;
}

}  // namespace clay
