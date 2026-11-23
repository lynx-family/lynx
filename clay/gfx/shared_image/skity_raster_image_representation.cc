// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skity_raster_image_representation.h"

#include "clay/gfx/color_space.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/skity/skity_image.h"

namespace clay {

SkityRasterImageRepresentation::SkityRasterImageRepresentation(
    fml::RefPtr<SharedImageBacking> backing)
    : SkityImageRepresentation(backing) {}

std::shared_ptr<SkityImage> SkityRasterImageRepresentation::GetSkityImage() {
  auto* backing = GetBacking();
  auto size = backing->GetSize();
  auto pixels = std::make_shared<skity::Pixmap>(
      size.x, size.y, skity::AlphaType::kPremul_AlphaType,
      backing->GetPixelFormat() == SharedImageBacking::PixelFormat::kNative8888
          ? skity::ColorType::kBGRA
          : skity::ColorType::kRGBA);
  pixels->SetColorInfo(pixels->GetAlphaType(), pixels->GetColorType(),
                       ToSkityColorSpace(backing->GetColorSpace()));
  if (!backing->ReadbackToMemory(pixels.get(), 1)) return nullptr;
  return std::make_shared<SkityImage>(skity::Image::MakeImage(pixels));
}

void SkityRasterImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence) {
  if (fence) fence->ClientWait();
}

std::unique_ptr<FenceSync> SkityRasterImageRepresentation::ProduceFence() {
  // Rendering retains an independent CPU copy, so the producer can reuse its
  // buffer.
  return nullptr;
}

}  // namespace clay
