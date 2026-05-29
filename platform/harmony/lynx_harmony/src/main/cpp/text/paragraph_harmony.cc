// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"

#include <native_drawing/drawing_pixel_map.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>

#include <algorithm>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/text/emoji_resource_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

void DrawInlineEmoji(OH_Drawing_Canvas* canvas, const InlineEmojiInfo& emoji,
                     float left, float top,
                     const std::weak_ptr<UIBase>& emoji_invalidate_target) {
  auto& emoji_resource_manager = EmojiResourceManager::GetInstance();
  auto* image = emoji_resource_manager.GetEmojiImage(emoji.name);
  if (!image) {
    return;
  }
  auto* draw_pixel_map = image->DrawPixelMap();
  if (!draw_pixel_map) {
    if (!emoji_invalidate_target.expired()) {
      emoji_resource_manager.DecodeEmojiImageIfNeeded(
          emoji.name, image, [emoji_invalidate_target] {
            if (auto target = emoji_invalidate_target.lock()) {
              target->Invalidate();
            }
          });
    }
    return;
  }

  if (image->Width() == 0 || image->Height() == 0) {
    return;
  }

  OH_Drawing_Rect* src_rect =
      OH_Drawing_RectCreate(0, 0, image->Width(), image->Height());
  OH_Drawing_Rect* dst_rect =
      OH_Drawing_RectCreate(left, top, left + emoji.width, top + emoji.height);
  OH_Drawing_SamplingOptions* sample =
      OH_Drawing_SamplingOptionsCreate(FILTER_MODE_LINEAR, MIPMAP_MODE_LINEAR);
  if (!src_rect || !dst_rect || !sample) {
    if (sample) {
      OH_Drawing_SamplingOptionsDestroy(sample);
    }
    if (dst_rect) {
      OH_Drawing_RectDestroy(dst_rect);
    }
    if (src_rect) {
      OH_Drawing_RectDestroy(src_rect);
    }
    return;
  }

  OH_Drawing_CanvasDrawPixelMapRect(canvas, draw_pixel_map, src_rect, dst_rect,
                                    sample);
  OH_Drawing_SamplingOptionsDestroy(sample);
  OH_Drawing_RectDestroy(dst_rect);
  OH_Drawing_RectDestroy(src_rect);
}

}  // namespace

void ParagraphHarmony::Draw(OH_Drawing_Canvas* canvas, const double x,
                            const double y) const {
  if (!canvas) {
    return;
  }
  OH_Drawing_TypographyPaint(paragraph_, canvas, x, y);
  if (inline_emojis_.empty()) {
    return;
  }

  auto rects = GetRectsForPlaceholders();
  if (rects.GetCount() == 0) {
    return;
  }

  for (const auto& emoji : inline_emojis_) {
    if (emoji.placeholder_index >= rects.GetCount()) {
      continue;
    }

    float left = static_cast<float>(x) + rects.GetLeft(emoji.placeholder_index);
    float top = static_cast<float>(y) + rects.GetTop(emoji.placeholder_index);
    float bottom =
        static_cast<float>(y) + rects.GetBottom(emoji.placeholder_index);
    if (bottom > top) {
      top = top + std::max(0.f, (bottom - top - emoji.height) / 2.f);
    }
    DrawInlineEmoji(canvas, emoji, left, top, emoji_invalidate_target_);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
