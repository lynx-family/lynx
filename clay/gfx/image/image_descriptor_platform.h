// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_PLATFORM_H_
#define CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_PLATFORM_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/synchronization/shared_mutex.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/gfx/image/platform_image.h"

namespace clay {

class ImageDescriptorPlatform : public ImageDescriptor {
 public:
  ImageDescriptorPlatform(GrDataPtr buffer,
                          std::shared_ptr<PlatformImage> image);
  ImageDescriptorPlatform(GrDataPtr buffer, const GrImageInfo& image_info,
                          std::shared_ptr<PlatformImage> image);

  static fml::RefPtr<ImageDescriptor> Create(
      std::shared_ptr<PlatformImage> codec);

  ~ImageDescriptorPlatform() = default;

  ImageDescriptorPlatform(const ImageDescriptorPlatform&) = delete;
  const ImageDescriptorPlatform& operator=(const ImageDescriptorPlatform&) =
      delete;

  // Associates a Clay Codec object.
  fml::RefPtr<Codec> InstantiateCodec(int target_width,
                                      int target_height) override;

  // Whether this descriptor represents compressed (encoded) data or not.
  bool IsCompressed() const override { return false; }

  fml::RefPtr<GraphicsImage> image() const override { return nullptr; }

  // Gets the scaled dimensions of this image, if backed by a codec that can
  // perform efficient subpixel scaling.
  skity::Vec2 GetScaledDimensions(float scale) override {
    return {image_info_.width() * scale, image_info_.height() * scale};
  }

  std::shared_ptr<PlatformImage> GetPlatformImage() { return image_; }

 private:
  std::shared_ptr<PlatformImage> image_;
  FML_FRIEND_MAKE_REF_COUNTED(ImageDescriptorPlatform);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(ImageDescriptorPlatform);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_DESCRIPTOR_PLATFORM_H_
