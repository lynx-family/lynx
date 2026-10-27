// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_DECODING_IMAGE_H_
#define CLAY_GFX_IMAGE_DECODING_IMAGE_H_

#include <functional>
#include <memory>

#include "clay/gfx/image/graphics_image_skia.h"
#include "clay/gfx/image/image.h"
#include "clay/gfx/image/image_resource.h"
#include "clay/gfx/image/image_resource_client.h"

namespace clay {

class DecodingImage final : public GraphicsImageSkia,
                            public ImageResourceClient {
 public:
  explicit DecodingImage(std::shared_ptr<Image> image);
  ~DecodingImage() override;

  int GetWidth() const;
  int GetHeight() const;

 private:
  void ScheduleDecodeAndUpload(
      const LazyImageDecodeCallback& callback) override;

  bool MaybeAnimated() const override;

  skity::Vec2 dimensions() const override;
  bool WillRenderImage() override { return true; }
  void RequestRenderImage(ImageResource* image_resource,
                          bool success) override {}
  void OnImageChanged() override {}
  void DecodeImageFinish(bool success) override;

  LazyImageDecodeCallback callback_;
  std::shared_ptr<Image> raw_image_ = nullptr;
  std::unique_ptr<ImageResource> image_resource_;

  BASE_DISALLOW_COPY_AND_ASSIGN(DecodingImage);
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_DECODING_IMAGE_H_
