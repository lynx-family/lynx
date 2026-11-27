// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skia_raster_image_representation.h"

#include <algorithm>
#include <utility>

#include "clay/gfx/color_space.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace clay {

SkiaRasterImageRepresentation::SkiaRasterImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SkiaImageRepresentation(std::move(backing)) {}

sk_sp<SkImage> SkiaRasterImageRepresentation::GetSkImage() {
  if (HasFailedFence()) return nullptr;
  auto* backing = GetBacking();
  auto size = GetSize();
  auto format = backing->GetPixelFormat();
  if (format != SharedImageBacking::PixelFormat::kNative8888 &&
      format != SharedImageBacking::PixelFormat::kRGBA8888) {
    return nullptr;
  }
  auto info = SkImageInfo::Make(
      size.x, size.y,
      format == SharedImageBacking::PixelFormat::kNative8888
          ? kBGRA_8888_SkColorType
          : kRGBA_8888_SkColorType,
      kPremul_SkAlphaType, ToSkColorSpace(backing->GetColorSpace()));
  SkBitmap bitmap;
  SkPixmap pixmap;
  if (!bitmap.tryAllocPixels(info) || !bitmap.peekPixels(&pixmap) ||
      !backing->ReadbackToMemory(&pixmap, 1)) {
    return nullptr;
  }
  auto* pixels = static_cast<uint8_t*>(bitmap.getPixels());
  for (int y = 0; y < info.height() / 2; ++y) {
    auto* row = pixels + y * bitmap.rowBytes();
    auto* opposite = pixels + (info.height() - y - 1) * bitmap.rowBytes();
    std::swap_ranges(row, row + bitmap.rowBytes(), opposite);
  }
  // Sharing immutable owned storage avoids a second full-frame copy.
  bitmap.setImmutable();
  return SkImages::RasterFromBitmap(bitmap);
}

void SkiaRasterImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence) {
  WaitForFence(std::move(fence));
}

std::unique_ptr<FenceSync> SkiaRasterImageRepresentation::ProduceFence() {
  return HasFailedFence() ? TakeFailedFence() : nullptr;
}

}  // namespace clay
