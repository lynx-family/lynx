// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_SKITY_DECODING_IMAGE_H_
#define CLAY_GFX_SKITY_SKITY_DECODING_IMAGE_H_

#include <functional>
#include <memory>

#include "clay/gfx/image/image.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/image/image_resource_client.h"
#include "skity/gpu/texture.hpp"
#include "skity/graphic/alpha_type.hpp"
#include "skity/graphic/image.hpp"
#include "skity/graphic/sampling_options.hpp"
#include "skity/io/pixmap.hpp"

namespace clay {

// The wrapper of skity image supports deferred image decode.
class SkityDecodingImage final : public skity::Image,
                                 public ImageResourceClient {
 public:
  using LazyImageDecodeCallback = std::function<void(bool)>;

  explicit SkityDecodingImage(std::shared_ptr<clay::Image> image);
  ~SkityDecodingImage() override;

  void ScheduleDecodeAndUpload(const LazyImageDecodeCallback& callback);

  std::shared_ptr<skity::Image> gr_image() const { return decoded_image_; }

  int GetWidth() const;
  int GetHeight() const;
  bool MaybeAnimated() const;

  // Implement functons of skity::image.
  bool IsTextureBackend() const override;
  const std::shared_ptr<skity::Texture>* GetTexture() const override;
  const std::shared_ptr<skity::Pixmap>* GetPixmap() const override;
  size_t Width() const override;
  size_t Height() const override;
  skity::AlphaType GetAlphaType() const override;
  bool ScalePixels(std::shared_ptr<skity::Pixmap> dst,
                   skity::GPUContext* context,
                   const GrSamplingOptions& sampling_options) const override;
  skity::ImageType GetImageType() const override {
    return skity::ImageType::kCustom;
  }

  // Implement functons of ImageResourceClient
  bool WillRenderImage() override { return true; }
  void RequestRenderImage(ImageResource* image_resource,
                          bool success) override {}
  void OnImageChanged() override {}
  void DecodeImageFinish(bool success) override;

 private:
  LazyImageDecodeCallback callback_;
  std::shared_ptr<clay::Image> raw_image_;
  std::shared_ptr<skity::Image> decoded_image_;
  std::unique_ptr<ImageResource> image_resource_;
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_SKITY_DECODING_IMAGE_H_
