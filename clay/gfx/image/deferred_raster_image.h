// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_DEFERRED_RASTER_IMAGE_H_
#define CLAY_GFX_IMAGE_DEFERRED_RASTER_IMAGE_H_

#include <atomic>
#include <memory>
#include <utility>

#include "skity/graphic/image.hpp"

namespace clay {

class DeferredRasterImage final : public skity::Image {
 public:
  DeferredRasterImage(
      size_t width, size_t height,
      skity::AlphaType alpha_type = skity::AlphaType::kPremul_AlphaType)
      : width_(width), height_(height), alpha_type_(alpha_type) {}

  // Each deferred image is filled by a single upload task.
  void SetPixmap(std::shared_ptr<skity::Pixmap> pixmap) {
    pixmap_ = std::move(pixmap);
    ready_.store(true, std::memory_order_release);
  }

  bool IsTextureBackend() const override { return false; }

  const std::shared_ptr<skity::Texture>* GetTexture() const override {
    return nullptr;
  }

  const std::shared_ptr<skity::Pixmap>* GetPixmap() const override {
    if (ready_.load(std::memory_order_acquire)) {
      return &pixmap_;
    }
    // The software canvas needs a sampleable pixel even if decoding fails.
    static const auto transparent = std::make_shared<skity::Pixmap>(
        1, 1, skity::AlphaType::kPremul_AlphaType, skity::ColorType::kRGBA);
    return &transparent;
  }

  size_t Width() const override { return width_; }
  size_t Height() const override { return height_; }
  skity::AlphaType GetAlphaType() const override { return alpha_type_; }
  skity::ImageType GetImageType() const override {
    return skity::ImageType::kPixmap;
  }

 private:
  const size_t width_;
  const size_t height_;
  const skity::AlphaType alpha_type_;
  std::shared_ptr<skity::Pixmap> pixmap_;
  std::atomic<bool> ready_{false};
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_DEFERRED_RASTER_IMAGE_H_
