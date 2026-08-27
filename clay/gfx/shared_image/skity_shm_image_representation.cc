// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skity_shm_image_representation.h"

#include <utility>

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/skity/skity_image.h"
#include "skity/graphic/image.hpp"
#include "skity/io/data.hpp"
#include "skity/io/pixmap.hpp"

namespace clay {

SkityShmImageRepresentation::SkityShmImageRepresentation(
    fml::RefPtr<SharedImageRepresentation> representation)
    : SkityImageRepresentation(fml::Ref(representation->GetBacking())),
      representation_(std::move(representation)) {}

SkityShmImageRepresentation::~SkityShmImageRepresentation() = default;

std::shared_ptr<SkityImage> SkityShmImageRepresentation::GetSkityImage() {
  ClaySharedImageReadResult result{};
  if (!representation_->BeginRead(&result) ||
      result.type != kClaySharedImageRepresentationTypeShm) {
    return nullptr;
  }

  const auto size = GetSize();
  const size_t bytes = static_cast<size_t>(size.x) * size.y * 4;
  auto data = skity::Data::MakeWithCopy(GetBacking()->GetGFXHandle(), bytes);
  if (result.shm_image.destruction_callback) {
    result.shm_image.destruction_callback(result.shm_image.user_data);
  }
  if (!data) {
    return nullptr;
  }
  auto pixmap = std::make_shared<skity::Pixmap>(
      std::move(data), static_cast<size_t>(size.x) * 4,
      static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y),
      skity::AlphaType::kPremul_AlphaType, skity::ColorType::kRGBA);
  auto image = skity::Image::MakeImage(std::move(pixmap));
  return image ? std::make_shared<SkityImage>(std::move(image)) : nullptr;
}

bool SkityShmImageRepresentation::EndRead() {
  return representation_->EndRead();
}

void SkityShmImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  representation_->ConsumeFence(std::move(fence_sync));
}

std::unique_ptr<FenceSync> SkityShmImageRepresentation::ProduceFence() {
  return representation_->ProduceFence();
}

}  // namespace clay
