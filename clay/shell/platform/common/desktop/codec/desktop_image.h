// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_H_
#define CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_H_

#include <memory>

#include "clay/gfx/geometry/size.h"
#include "clay/gfx/image/image_info.h"
#include "clay/gfx/image/platform_image.h"
#include "skity/codec/codec.hpp"

namespace clay {

class DesktopImage : public PlatformImage {
 public:
  DesktopImage(std::shared_ptr<skity::Codec> codec, Size decode_size);
  ~DesktopImage() override;

  int GetWidth() override;
  int GetHeight() override;
  skity::ColorType GetColorType() override;
  skity::AlphaType GetAlphaType() override;
  std::shared_ptr<skity::Pixmap> ToBitmap(
      const ImageInfo& render_info) override;
  bool IsAnimated() override;
  std::unique_ptr<PlatformImageAnimation> CreateAnimation() override;

 private:
  int width_ = 0;
  int height_ = 0;
  bool is_animated_ = false;
  bool decoded_to_target_size_ = false;
  skity::ColorType color_type_ = skity::ColorType::kUnknown;
  skity::AlphaType alpha_type_ = skity::AlphaType::kUnknown_AlphaType;
  std::shared_ptr<skity::Codec> codec_;
  std::shared_ptr<skity::Pixmap> current_pixmap_;
  std::shared_ptr<skity::MultiFrameDecoder> decoder_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_DESKTOP_CODEC_DESKTOP_IMAGE_H_
