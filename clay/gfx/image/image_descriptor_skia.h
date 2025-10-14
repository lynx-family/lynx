// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_SKIA_H_
#define CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_SKIA_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "clay/gfx/image/image_descriptor.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkImageGenerator.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/src/codec/SkCodecImageGenerator.h"  // nogncheck

namespace clay {

class ImageDescriptorSkia : public ImageDescriptor {
 public:
  enum PixelFormat {
    kRGBA8888,
    kBGRA8888,
    kRGB565,
  };

  ImageDescriptorSkia(sk_sp<SkData> buffer, std::unique_ptr<SkCodec> codec);
  ImageDescriptorSkia(sk_sp<SkData> buffer, const SkImageInfo& image_info,
                      std::unique_ptr<SkCodec> codec);
  ImageDescriptorSkia(sk_sp<SkData> buffer,
                      std::unique_ptr<SkImageGenerator> generator);
  ImageDescriptorSkia(sk_sp<SkData> buffer, const SkImageInfo& image_info,
                      std::optional<size_t> row_bytes);

  static fml::RefPtr<ImageDescriptorSkia> Create(sk_sp<SkData> data, int width,
                                                 int height, int row_bytes,
                                                 PixelFormat pixel_format);

  ~ImageDescriptorSkia() = default;

  ImageDescriptorSkia(const ImageDescriptorSkia&) = delete;
  const ImageDescriptorSkia& operator=(const ImageDescriptorSkia&) = delete;

  // Associates a Clay Codec object.
  fml::RefPtr<Codec> InstantiateCodec(int target_width,
                                      int target_height) override;

  fml::RefPtr<GraphicsImage> image() const override;

  // Whether this descriptor represents compressed (encoded) data or not.
  bool IsCompressed() const override {
    return generator_ || platform_image_generator_;
  }

  // Gets the scaled dimensions of this image, if backed by a codec that can
  // perform efficient subpixel scaling.
  skity::Vec2 GetScaledDimensions(float scale) override {
    if (generator_) {
      return {
          static_cast<float>(generator_->getScaledDimensions(scale).width()),
          static_cast<float>(generator_->getScaledDimensions(scale).height())};
    }
    return {static_cast<float>(image_info_.width()),
            static_cast<float>(image_info_.height())};
  }

  // Gets pixels for this image transformed based on the EXIF orientation tag,
  // if applicable.
  bool GetPixels(const SkPixmap& pixmap) const override;

  std::shared_ptr<SkCodecImageGenerator> GetGenerator() { return generator_; }

 private:
  const SkImageInfo CreateImageInfo() const;
  SkAlphaType GetCodecAlphaType() const;
  fml::RefPtr<GraphicsImage> Get565Image() const;

  std::shared_ptr<SkCodecImageGenerator> generator_;
  std::unique_ptr<SkImageGenerator> platform_image_generator_;

  FML_FRIEND_MAKE_REF_COUNTED(ImageDescriptorSkia);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(ImageDescriptorSkia);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_SKIA_H_
