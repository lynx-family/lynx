// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/offscreen_surface.h"

#include "clay/fml/logging.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkImageEncoder.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkSurfaceCharacterization.h"
#include "third_party/skia/include/utils/SkBase64.h"

namespace clay {

static sk_sp<SkSurface> CreateSnapshotSurface(GrDirectContext* surface_context,
                                              const skity::Vec2& size,
                                              bool opaque) {
  const auto image_info = SkImageInfo::Make(
      {static_cast<int32_t>(size.x), static_cast<int32_t>(size.y)},
      kRGBA_8888_SkColorType,
      opaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType, nullptr);
  if (surface_context) {
    // There is a rendering surface that may contain textures that are going to
    // be referenced in the layer tree about to be drawn.
    return SkSurface::MakeRenderTarget(
        reinterpret_cast<GrRecordingContext*>(surface_context),
        skgpu::Budgeted::kNo, image_info);
  }

  // There is no rendering surface, assume no GPU textures are present and
  // create a raster surface.
  return SkSurface::MakeRaster(image_info);
}

/// Returns a buffer containing a snapshot of the surface.
///
/// If compressed is true the data is encoded as PNG.
static sk_sp<SkData> GetRasterData(const sk_sp<SkSurface>& offscreen_surface,
                                   bool compressed) {
  // Prepare an image from the surface, this image may potentially be on th GPU.
  auto potentially_gpu_snapshot = offscreen_surface->makeImageSnapshot();
  if (!potentially_gpu_snapshot) {
    FML_DLOG(ERROR) << "Screenshot: unable to make image screenshot";
    return nullptr;
  }

  // Copy the GPU image snapshot into CPU memory.
  // TODO (https://github.com/flutter/flutter/issues/13498) // NOLINT
  auto cpu_snapshot = potentially_gpu_snapshot->makeRasterImage();
  if (!cpu_snapshot) {
    FML_DLOG(ERROR) << "Screenshot: unable to make raster image";
    return nullptr;
  }

  // If the caller want the pixels to be compressed, there is a Skia utility to
  // compress to PNG. Use that.
  if (compressed) {
    return cpu_snapshot->encodeToData();
  }

  // Copy it into a bitmap and return the same.
  SkPixmap pixmap;
  if (!cpu_snapshot->peekPixels(&pixmap)) {
    FML_DLOG(ERROR) << "Screenshot: unable to obtain bitmap pixels";
    return nullptr;
  }
  return SkData::MakeWithCopy(pixmap.addr32(), pixmap.computeByteSize());
}

OffscreenSurface::OffscreenSurface(GrDirectContext* surface_context,
                                   const skity::Vec2& size, bool opaque) {
  offscreen_surface_ = CreateSnapshotSurface(surface_context, size, opaque);
}

sk_sp<SkData> OffscreenSurface::GetRasterData(bool compressed) const {
  return clay::GetRasterData(offscreen_surface_, compressed);
}

sk_sp<SkImage> OffscreenSurface::GetRasterImage() const {
  auto gpu_snapshot = offscreen_surface_->makeImageSnapshot();
  if (!gpu_snapshot) {
    FML_DLOG(ERROR) << "Screenshot: unable to make image screenshot";
    return nullptr;
  }
  return gpu_snapshot->makeRasterImage();
}

SkCanvas* OffscreenSurface::GetCanvas() const {
  return offscreen_surface_->getCanvas();
}

bool OffscreenSurface::IsValid() const { return offscreen_surface_ != nullptr; }

}  // namespace clay
