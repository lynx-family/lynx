// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_INLINE_EMOJI_BITMAP_H_
#define CLAY_UI_COMPONENT_TEXT_INLINE_EMOJI_BITMAP_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/image/graphics_image.h"

namespace clay {

struct InlineEmojiBitmap {
  int width = 0;
  int height = 0;
  size_t row_bytes = 0;
  float layout_width = 0.f;
  float layout_height = 0.f;
  std::vector<uint8_t> pixels;

  bool IsValid() const {
    return width > 0 && height > 0 &&
           row_bytes >= static_cast<size_t>(width) * 4 &&
           pixels.size() >= row_bytes * static_cast<size_t>(height) &&
           layout_width > 0.f && layout_height > 0.f;
  }
};

struct InlineEmojiInfo {
  int placeholder_id = -1;
  InlineEmojiBitmap bitmap;
};

fml::RefPtr<GraphicsImage> CreateInlineEmojiGraphicsImage(
    const InlineEmojiBitmap& bitmap);

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_INLINE_EMOJI_BITMAP_H_
