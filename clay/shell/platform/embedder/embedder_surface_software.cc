// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/embedder/embedder_surface_software.h"

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "base/trace/native/trace_event.h"
#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#endif  // ENABLE_SKITY

namespace clay {

#ifdef ENABLE_SKITY
namespace {

class SoftwareSurface final : public skity::GPUSurface {
 public:
  SoftwareSurface(uint32_t width, uint32_t height)
      : bitmap_(width, height, skity::AlphaType::kPremul_AlphaType),
        canvas_(skity::Canvas::MakeSoftwareCanvas(&bitmap_)) {}

  bool IsValid() const { return bitmap_.GetPixelAddr() && canvas_; }
  uint32_t GetWidth() const override { return bitmap_.Width(); }
  uint32_t GetHeight() const override { return bitmap_.Height(); }
  float ContentScale() const override { return 1.f; }
  skity::Canvas* LockCanvas(bool clear) override {
    if (clear) {
      canvas_->Clear(skity::Color_TRANSPARENT);
    }
    return canvas_.get();
  }
  void Flush() override { canvas_->Flush(); }
  std::shared_ptr<skity::Pixmap> ReadPixels(const skity::Rect& rect) override {
    if (rect != skity::Rect::MakeWH(GetWidth(), GetHeight())) {
      return nullptr;
    }
    return bitmap_.GetPixmap();
  }
  const skity::Bitmap& GetBitmap() const { return bitmap_; }

 private:
  skity::Bitmap bitmap_;
  std::unique_ptr<skity::Canvas> canvas_;
};

}  // namespace
#endif

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
    clay::GrContextPtr) {
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
  if (!std::isfinite(skity_size.x) || !std::isfinite(skity_size.y) ||
      skity_size.x < 1 || skity_size.y < 1 ||
      static_cast<double>(skity_size.x) >
          std::numeric_limits<int32_t>::max() / 4 ||
      static_cast<double>(skity_size.y) > std::numeric_limits<int32_t>::max() ||
      static_cast<double>(skity_size.x) * skity_size.y * 4 >
          std::numeric_limits<size_t>::max() ||
      std::floor(skity_size.x) != skity_size.x ||
      std::floor(skity_size.y) != skity_size.y) {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  SkISize size = SkISize::Make(skity_size.x, skity_size.y);
  if (sk_surface_ != nullptr && sk_surface_->unique() &&
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
#else
  if (sk_surface_ && sk_surface_.use_count() == 1 &&
      sk_surface_->GetWidth() == skity_size.x &&
      sk_surface_->GetHeight() == skity_size.y) {
    return sk_surface_;
  }
  auto surface = std::make_shared<SoftwareSurface>(skity_size.x, skity_size.y);
  if (!surface->IsValid()) {
    return nullptr;
  }
  sk_surface_ = std::move(surface);
#endif
  return sk_surface_;
}

// |GPUSurfaceSoftwareDelegate|
bool EmbedderSurfaceSoftware::PresentBackingStore(
    clay::GrSurfacePtr backing_store) {
  if (!IsValid() || !backing_store) {
    return false;
  }
#ifdef ENABLE_SKITY
  const auto& bitmap =
      static_cast<SoftwareSurface*>(backing_store.get())->GetBitmap();
  return delegate_->OnPresentBackingStore(bitmap.GetPixelAddr(),
                                          bitmap.RowBytes(), bitmap.Height());
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
