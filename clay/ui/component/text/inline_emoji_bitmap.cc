// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/inline_emoji_bitmap.h"

#include <memory>
#include <utility>

namespace clay {

fml::RefPtr<GraphicsImage> CreateInlineEmojiGraphicsImage(
    const InlineEmojiBitmap& bitmap) {
  if (!bitmap.IsValid()) {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  auto data = SkData::MakeWithCopy(bitmap.pixels.data(), bitmap.pixels.size());
  auto info = SkImageInfo::Make(bitmap.width, bitmap.height,
                                kRGBA_8888_SkColorType, kPremul_SkAlphaType);
  return GraphicsImage::MakeRasterData(info, std::move(data), bitmap.row_bytes);
#else
  auto data =
      skity::Data::MakeWithCopy(bitmap.pixels.data(), bitmap.pixels.size());
  auto info = ImageInfo(bitmap.width, bitmap.height, kRGBA_8888_ColorType,
                        kPremul_AlphaType);
  return GraphicsImage::MakeRasterData(info, std::move(data), bitmap.row_bytes);
#endif
}

}  // namespace clay
