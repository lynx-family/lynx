// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"

#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "core/renderer/dom/fragment/rounded_rectangle.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

void SetDrawableRadius(BackgroundDrawable& drawable,
                       const RoundedRectangle& box) {
  auto* border_radius = drawable.GetBorderRadius();
  border_radius->SetRadius(BorderRadius::CornerPosition::kTopLeft,
                           box.GetRadiusXTopLeft(), box.GetRadiusYTopLeft());
  border_radius->SetRadius(BorderRadius::CornerPosition::kTopRight,
                           box.GetRadiusXTopRight(), box.GetRadiusYTopRight());
  border_radius->SetRadius(BorderRadius::CornerPosition::kBottomRight,
                           box.GetRadiusXBottomRight(),
                           box.GetRadiusYBottomRight());
  border_radius->SetRadius(BorderRadius::CornerPosition::kBottomLeft,
                           box.GetRadiusXBottomLeft(),
                           box.GetRadiusYBottomLeft());
}

void ConfigureFillDrawable(BackgroundDrawable& drawable,
                           const RoundedRectangle& box, uint32_t color,
                           float density) {
  drawable.SetBackgroundColor(color);
  SetDrawableRadius(drawable, box);
  drawable.UpdateBounds(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight(),
                        0.f, 0.f, 0.f, 0.f, density);
  drawable.AdjustBorder();
}

void ConfigureBorderDrawable(BackgroundDrawable& drawable,
                             const RoundedRectangle& outer,
                             const RoundedRectangle& inner,
                             const DisplayListItem::Payload& payload,
                             float density) {
  const float outer_right = outer.GetX() + outer.GetWidth();
  const float outer_bottom = outer.GetY() + outer.GetHeight();
  const float inner_right = inner.GetX() + inner.GetWidth();
  const float inner_bottom = inner.GetY() + inner.GetHeight();
  drawable.SetBorderWidth({std::max(inner.GetX() - outer.GetX(), 0.f),
                           std::max(outer_right - inner_right, 0.f),
                           std::max(inner.GetY() - outer.GetY(), 0.f),
                           std::max(outer_bottom - inner_bottom, 0.f)});

  drawable.SetBorderTopColor(lepus::Value(payload.border.colors[0]));
  drawable.SetBorderRightColor(lepus::Value(payload.border.colors[1]));
  drawable.SetBorderBottomColor(lepus::Value(payload.border.colors[2]));
  drawable.SetBorderLeftColor(lepus::Value(payload.border.colors[3]));
  drawable.SetBorderTopStyle(lepus::Value(payload.border.styles[0]));
  drawable.SetBorderRightStyle(lepus::Value(payload.border.styles[1]));
  drawable.SetBorderBottomStyle(lepus::Value(payload.border.styles[2]));
  drawable.SetBorderLeftStyle(lepus::Value(payload.border.styles[3]));

  SetDrawableRadius(drawable, outer);

  drawable.UpdateBounds(outer.GetX(), outer.GetY(), outer.GetWidth(),
                        outer.GetHeight(), 0.f, 0.f, 0.f, 0.f, density);
  drawable.AdjustBorder();
}

RoundedRectangle RecordBox(const DisplayListItem& item) {
  RoundedRectangle box;
  box.SetX(item.payload.record_box.x);
  box.SetY(item.payload.record_box.y);
  box.SetWidth(item.payload.record_box.w);
  box.SetHeight(item.payload.record_box.h);
  if (item.payload.record_box.has_radii) {
    box.SetRadiusXTopLeft(item.payload.record_box.radii[0]);
    box.SetRadiusYTopLeft(item.payload.record_box.radii[1]);
    box.SetRadiusXTopRight(item.payload.record_box.radii[2]);
    box.SetRadiusYTopRight(item.payload.record_box.radii[3]);
    box.SetRadiusXBottomRight(item.payload.record_box.radii[4]);
    box.SetRadiusYBottomRight(item.payload.record_box.radii[5]);
    box.SetRadiusXBottomLeft(item.payload.record_box.radii[6]);
    box.SetRadiusYBottomLeft(item.payload.record_box.radii[7]);
  }
  return box;
}

}  // namespace

LynxDisplayListApplier::LynxDisplayListApplier(LynxRendererContext* context)
    : context_(context) {}

LynxDisplayListApplier::~LynxDisplayListApplier() { Reset(); }

void LynxDisplayListApplier::UpdateDisplayListResources(
    const DisplayList& display_list,
    const std::shared_ptr<UIBase>& renderer_host) {
  if (context_ == nullptr || renderer_host == nullptr) {
    return;
  }

  auto previous_text_ids = std::move(active_text_ids_);
  auto previous_image_ids = std::move(active_image_ids_);
  boxes_.clear();
  active_text_ids_.clear();
  active_image_ids_.clear();

  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  for (size_t i = 0; i < display_list.GetContentItemsSize(); ++i) {
    const auto& item = items[i];
    if (item.type == DisplayListOpType::kRecordBox) {
      boxes_.emplace_back(RecordBox(item));
      continue;
    }
    if (item.type == DisplayListOpType::kImage) {
      const int32_t image_id = item.payload.image.image_id;
      active_image_ids_.insert(image_id);
      context_->RegisterImageManagerTarget(image_id, renderer_host);
      continue;
    }
    if (item.type != DisplayListOpType::kText) {
      continue;
    }

    const int32_t text_id = item.payload.text.text_id;
    active_text_ids_.insert(text_id);
    context_->RegisterPlatformExtraBundleTarget(text_id, renderer_host);
    auto text_bundle = context_->GetTextBundle(text_id);
    if (text_bundle == nullptr) {
      continue;
    }
    auto* paragraph = static_cast<ParagraphHarmony*>(text_bundle.get());
    paragraph->SetEmojiInvalidateTarget(renderer_host);
    for (const auto& layout : paragraph->GetInlinePlaceholderLayouts()) {
      if (!layout.is_image) {
        continue;
      }
      active_image_ids_.insert(layout.sign);
      context_->RegisterImageManagerTarget(layout.sign, renderer_host);
    }
  }

  for (int32_t text_id : previous_text_ids) {
    if (active_text_ids_.count(text_id) == 0) {
      context_->UnregisterPlatformExtraBundleTarget(text_id);
    }
  }
  for (int32_t image_id : previous_image_ids) {
    if (active_image_ids_.count(image_id) == 0) {
      context_->UnregisterImageManagerTarget(image_id);
    }
  }
}

void LynxDisplayListApplier::DrawSegment(
    const DisplayList& display_list, const DisplayListSegment& segment,
    const std::shared_ptr<UIBase>& renderer_host, OH_Drawing_Canvas* canvas) {
  if (context_ == nullptr || renderer_host == nullptr || canvas == nullptr ||
      segment.IsEmpty()) {
    return;
  }
  ProcessContentOperations(display_list, segment, renderer_host, canvas,
                           segment.start_item_index == 0);
}

void LynxDisplayListApplier::Reset() {
  if (context_ != nullptr) {
    for (int32_t text_id : active_text_ids_) {
      context_->UnregisterPlatformExtraBundleTarget(text_id);
    }
    for (int32_t image_id : active_image_ids_) {
      context_->UnregisterImageManagerTarget(image_id);
    }
  }
  boxes_.clear();
  active_text_ids_.clear();
  active_image_ids_.clear();
}

void LynxDisplayListApplier::DrawInlineImages(
    ParagraphHarmony* paragraph, const std::shared_ptr<UIBase>& renderer_host,
    OH_Drawing_Canvas* canvas, float density) {
  if (paragraph == nullptr || renderer_host == nullptr || canvas == nullptr ||
      density <= 0.f) {
    return;
  }

  for (const auto& layout : paragraph->GetInlinePlaceholderLayouts()) {
    if (!layout.is_image || layout.width <= 0.f || layout.height <= 0.f) {
      continue;
    }

    if (active_image_ids_.insert(layout.sign).second) {
      context_->RegisterImageManagerTarget(layout.sign, renderer_host);
    }
    auto image_manager = context_->GetImageManager(layout.sign);
    if (image_manager == nullptr) {
      continue;
    }

    OH_Drawing_CanvasSave(canvas);
    OH_Drawing_CanvasTranslate(canvas, layout.left, layout.top);
    auto* clip_rect =
        OH_Drawing_RectCreate(0.f, 0.f, layout.width, layout.height);
    OH_Drawing_CanvasClipRect(canvas, clip_rect,
                              OH_Drawing_CanvasClipOp::INTERSECT, true);
    OH_Drawing_RectDestroy(clip_rect);

    image_manager->UpdateBounds(layout.width / density, layout.height / density,
                                density);
    image_manager->Draw(canvas);
    OH_Drawing_CanvasRestore(canvas);
  }
}

void LynxDisplayListApplier::ProcessContentOperations(
    const DisplayList& display_list, const DisplayListSegment& segment,
    const std::shared_ptr<UIBase>& renderer_host, OH_Drawing_Canvas* canvas,
    bool skip_first_translate) {
  if (renderer_host == nullptr || segment.IsEmpty()) {
    return;
  }
  auto* lynx_context = context_->GetLynxContext();
  if (lynx_context == nullptr) {
    return;
  }
  const float density = lynx_context->ScaledDensity();
  int32_t fragment_depth = 0;
  bool has_seen_first_begin = false;
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());

  OH_Drawing_CanvasSave(canvas);
  for (size_t i = segment.start_item_index; i < segment.end_item_index; ++i) {
    const auto& item = items[i];
    switch (item.type) {
      case DisplayListOpType::kBegin: {
        OH_Drawing_CanvasSave(canvas);
        if (!(skip_first_translate && !has_seen_first_begin)) {
          OH_Drawing_CanvasTranslate(canvas, item.payload.begin.x * density,
                                     item.payload.begin.y * density);
        }
        has_seen_first_begin = true;
        ++fragment_depth;
        break;
      }
      case DisplayListOpType::kEnd: {
        if (fragment_depth > 0) {
          OH_Drawing_CanvasRestore(canvas);
          --fragment_depth;
        }
        break;
      }
      case DisplayListOpType::kFill: {
        const int32_t clip_index = item.payload.fill.clip_index;
        if (clip_index < 0 ||
            static_cast<size_t>(clip_index) >= boxes_.size()) {
          break;
        }
        if (!fill_drawable_) {
          fill_drawable_ = std::make_unique<BackgroundDrawable>(
              std::weak_ptr<UIBase>(), false);
        }
        ConfigureFillDrawable(*fill_drawable_, boxes_[clip_index],
                              item.payload.fill.color, density);
        fill_drawable_->Render(canvas);
        break;
      }
      case DisplayListOpType::kDrawView: {
        // TODO: Support draw-view ordering if Harmony fragment layer needs it.
        break;
      }
      case DisplayListOpType::kText: {
        const int32_t text_id = item.payload.text.text_id;
        auto text_bundle = context_->GetTextBundle(text_id);
        if (text_bundle == nullptr) {
          break;
        }
        auto* paragraph = static_cast<ParagraphHarmony*>(text_bundle.get());
        if (paragraph == nullptr) {
          break;
        }
        paragraph->SetEmojiInvalidateTarget(renderer_host);
        paragraph->Draw(canvas, paragraph->GetTranslateLeftOffset(), 0.f);
        DrawInlineImages(paragraph, renderer_host, canvas, density);
        break;
      }
      case DisplayListOpType::kImage: {
        int32_t image_id = item.payload.image.image_id;
        int32_t box_index = item.payload.image.box_index;
        if (box_index < 0 || static_cast<size_t>(box_index) >= boxes_.size()) {
          break;
        }
        const auto& box = boxes_[box_index];
        const float x = box.GetX();
        const float y = box.GetY();
        const float width = box.GetWidth();
        const float height = box.GetHeight();
        std::shared_ptr<LynxImageManager> image_manager =
            context_->GetImageManager(image_id);
        if (!image_manager || width <= 0.f || height <= 0.f) {
          break;
        }

        OH_Drawing_CanvasSave(canvas);
        OH_Drawing_CanvasTranslate(canvas, x * density, y * density);
        auto* clip_rect =
            OH_Drawing_RectCreate(0.f, 0.f, width * density, height * density);
        OH_Drawing_CanvasClipRect(canvas, clip_rect,
                                  OH_Drawing_CanvasClipOp::INTERSECT, true);
        OH_Drawing_RectDestroy(clip_rect);

        image_manager->UpdateBounds(width, height, density);
        image_manager->Draw(canvas);
        OH_Drawing_CanvasRestore(canvas);
        break;
      }
      case DisplayListOpType::kCustom: {
        // TODO: Support custom drawing on Harmony fragment layer.
        break;
      }
      case DisplayListOpType::kBorder: {
        const int32_t outer_index = item.payload.border.out_index;
        const int32_t inner_index = item.payload.border.inner_index;
        if (outer_index < 0 || inner_index < 0 ||
            static_cast<size_t>(outer_index) >= boxes_.size() ||
            static_cast<size_t>(inner_index) >= boxes_.size()) {
          break;
        }
        if (!border_drawable_) {
          border_drawable_ = std::make_unique<BackgroundDrawable>(
              std::weak_ptr<UIBase>(), false);
        }
        ConfigureBorderDrawable(*border_drawable_, boxes_[outer_index],
                                boxes_[inner_index], item.payload, density);
        border_drawable_->Render(canvas);
        break;
      }
      case DisplayListOpType::kClipRect: {
        const auto& clip = item.payload.clip_rect;
        auto* clip_rect = OH_Drawing_RectCreate(
            clip.x * density, clip.y * density, (clip.x + clip.w) * density,
            (clip.y + clip.h) * density);
        if (!clip.has_radii) {
          OH_Drawing_CanvasClipRect(canvas, clip_rect,
                                    OH_Drawing_CanvasClipOp::INTERSECT, true);
          OH_Drawing_RectDestroy(clip_rect);
          break;
        }

        auto* round_rect = OH_Drawing_RoundRectCreate(clip_rect, 0.f, 0.f);
        OH_Drawing_RectDestroy(clip_rect);
        constexpr OH_Drawing_CornerPos kCornerPositions[] = {
            CORNER_POS_TOP_LEFT, CORNER_POS_TOP_RIGHT, CORNER_POS_BOTTOM_RIGHT,
            CORNER_POS_BOTTOM_LEFT};
        for (size_t corner = 0; corner < 4; ++corner) {
          OH_Drawing_Corner_Radii radii{clip.radii[corner * 2] * density,
                                        clip.radii[corner * 2 + 1] * density};
          OH_Drawing_RoundRectSetCorner(round_rect, kCornerPositions[corner],
                                        radii);
        }
        OH_Drawing_CanvasClipRoundRect(
            canvas, round_rect, OH_Drawing_CanvasClipOp::INTERSECT, true);
        OH_Drawing_RoundRectDestroy(round_rect);
        break;
      }
      case DisplayListOpType::kRecordBox: {
        // Box indices are global to the complete display list. The boxes are
        // collected before drawing so every segment resolves the same index.
        break;
      }
      case DisplayListOpType::kLinearGradient: {
        // TODO: Support linear-gradient drawing on Harmony fragment layer.
        break;
      }
      case DisplayListOpType::kBoxShadow: {
        // TODO: Support box-shadow drawing on Harmony fragment layer.
        break;
      }
      case DisplayListOpType::kBackgroundImage: {
        // TODO: Support background-image drawing on Harmony fragment layer.
        break;
      }
      default: {
        break;
      }
    }
  }

  while (fragment_depth > 0) {
    OH_Drawing_CanvasRestore(canvas);
    --fragment_depth;
  }
  OH_Drawing_CanvasRestore(canvas);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
