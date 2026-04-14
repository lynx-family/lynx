// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/embedder_surface_software.h"

#include <memory>
#include <utility>

#include "base/trace/native/trace_event.h"
#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#endif  // ENABLE_SKITY

namespace clay {

EmbedderSurfaceSoftware::EmbedderSurfaceSoftware(
    EmbedderSurfaceSoftwareDelegate* delegate)
    : delegate_(delegate) {
  if (!delegate_) {
    return;
  }
  valid_ = true;
}

EmbedderSurfaceSoftware::~EmbedderSurfaceSoftware() = default;

// |OutputSurface|
bool EmbedderSurfaceSoftware::IsValid() const { return valid_; }

// |OutputSurface|
std::unique_ptr<Surface> EmbedderSurfaceSoftware::CreateGPUSurface(
    clay::GrContext*) {
  if (!IsValid()) {
    return nullptr;
  }
  auto surface = std::make_unique<GPUSurfaceSoftware>(this, true);

  if (!surface->IsValid()) {
    return nullptr;
  }

  return surface;
}

// |GPUSurfaceSoftwareDelegate|
clay::GrSurfacePtr EmbedderSurfaceSoftware::AcquireBackingStore(
    const skity::Vec2& skity_size) {
  TRACE_EVENT("clay", "EmbedderSurfaceSoftware::AcquireBackingStore");
  if (!IsValid()) {
    FML_LOG(ERROR)
        << "Could not acquire backing store for the software surface.";
    return nullptr;
  }
#ifndef ENABLE_SKITY
  SkISize size = SkISize::Make(skity_size.x, skity_size.y);
  if (sk_surface_ != nullptr &&
      SkISize::Make(sk_surface_->width(), sk_surface_->height()) == size) {
    // The old and new surface sizes are the same. Nothing to do here.
    return sk_surface_;
  }

  SkImageInfo info = SkImageInfo::MakeN32(
      size.fWidth, size.fHeight, kPremul_SkAlphaType, SkColorSpace::MakeSRGB());
  sk_surface_ = SkSurface::MakeRaster(info, nullptr);

  if (sk_surface_ == nullptr) {
    FML_LOG(ERROR) << "Could not create backing store for software rendering.";
    return nullptr;
  }
#endif
  return sk_surface_;
}

// |GPUSurfaceSoftwareDelegate|
bool EmbedderSurfaceSoftware::PresentBackingStore(
    clay::GrSurfacePtr backing_store) {
#ifdef ENABLE_SKITY
  FML_UNIMPLEMENTED();
  return false;
#else
  if (!IsValid()) {
    FML_LOG(ERROR) << "Tried to present an invalid software surface.";
    return false;
  }

  SkPixmap pixmap;
  if (!backing_store->peekPixels(&pixmap)) {
    FML_LOG(ERROR) << "Could not peek the pixels of the backing store.";
    return false;
  }

  // Some basic sanity checking.
  uint64_t expected_pixmap_data_size = pixmap.width() * pixmap.height() * 4;

  const size_t pixmap_size = pixmap.computeByteSize();

  if (expected_pixmap_data_size != pixmap_size) {
    FML_LOG(ERROR) << "Software backing store had unexpected size.";
    return false;
  }

  return delegate_->OnPresentBackingStore(pixmap.addr(),      //
                                          pixmap.rowBytes(),  //
                                          pixmap.height());   //
#endif
}

}  // namespace clay
