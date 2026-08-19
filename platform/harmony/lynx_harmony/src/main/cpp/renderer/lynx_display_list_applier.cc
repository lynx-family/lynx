// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"

#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

void SetDrawableRadius(BackgroundDrawable& drawable,
                       const RoundedRectangle& box) {
  auto* radius = drawable.GetBorderRadius();
  radius->SetRadius(BorderRadius::CornerPosition::kTopLeft,
                    box.GetRadiusXTopLeft(), box.GetRadiusYTopLeft());
  radius->SetRadius(BorderRadius::CornerPosition::kTopRight,
                    box.GetRadiusXTopRight(), box.GetRadiusYTopRight());
  radius->SetRadius(BorderRadius::CornerPosition::kBottomRight,
                    box.GetRadiusXBottomRight(), box.GetRadiusYBottomRight());
  radius->SetRadius(BorderRadius::CornerPosition::kBottomLeft,
                    box.GetRadiusXBottomLeft(), box.GetRadiusYBottomLeft());
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

LynxDisplayListApplier::~LynxDisplayListApplier() = default;

void LynxDisplayListApplier::UpdateDisplayListResources(
    const DisplayList& display_list) {
  boxes_.clear();
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  for (size_t i = 0; i < display_list.GetContentItemsSize(); ++i) {
    if (items[i].type == DisplayListOpType::kRecordBox) {
      boxes_.emplace_back(RecordBox(items[i]));
    }
  }
}

void LynxDisplayListApplier::DrawSegment(
    const DisplayList& display_list,
    const DisplayListSegmentResult& segment_result,
    const DisplayListSegment& segment,
    const std::shared_ptr<UIBase>& renderer_host, OH_Drawing_Canvas* canvas) {
  if (context_ == nullptr || renderer_host == nullptr || canvas == nullptr ||
      segment.ItemCount() == 0) {
    return;
  }
  ProcessContentOperations(display_list, segment_result, segment, renderer_host,
                           canvas, segment.start_item_index == 0);
}

void LynxDisplayListApplier::Reset() {
  boxes_.clear();
  replay_item_indices_.clear();
  fill_drawable_.reset();
  border_drawable_.reset();
}

void LynxDisplayListApplier::ProcessContentOperations(
    const DisplayList& display_list,
    const DisplayListSegmentResult& segment_result,
    const DisplayListSegment& segment,
    const std::shared_ptr<UIBase>& renderer_host, OH_Drawing_Canvas* canvas,
    bool skip_first_translate) {
  auto lynx_context = context_->GetLynxContext();
  if (lynx_context == nullptr) {
    return;
  }
  const float density = lynx_context->ScaledDensity();
  int32_t fragment_depth = 0;
  bool has_seen_first_begin = false;
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());

  OH_Drawing_CanvasSave(canvas);
  auto apply_begin = [&](const DisplayListItem& item, bool skip_translate) {
    OH_Drawing_CanvasSave(canvas);
    if (!skip_translate) {
      OH_Drawing_CanvasTranslate(canvas, item.payload.begin.x * density,
                                 item.payload.begin.y * density);
    }
    has_seen_first_begin = true;
    ++fragment_depth;
  };
  auto apply_clip_rect = [&](const DisplayListItem& item) {
    const auto& clip = item.payload.clip_rect;
    auto* rect = OH_Drawing_RectCreate(clip.x * density, clip.y * density,
                                       (clip.x + clip.w) * density,
                                       (clip.y + clip.h) * density);
    if (!clip.has_radii) {
      OH_Drawing_CanvasClipRect(canvas, rect,
                                OH_Drawing_CanvasClipOp::INTERSECT, true);
      OH_Drawing_RectDestroy(rect);
      return;
    }
    auto* round_rect = OH_Drawing_RoundRectCreate(rect, 0.f, 0.f);
    OH_Drawing_RectDestroy(rect);
    constexpr OH_Drawing_CornerPos kCornerPositions[] = {
        CORNER_POS_TOP_LEFT, CORNER_POS_TOP_RIGHT, CORNER_POS_BOTTOM_RIGHT,
        CORNER_POS_BOTTOM_LEFT};
    for (size_t corner = 0; corner < 4; ++corner) {
      OH_Drawing_Corner_Radii radii{clip.radii[corner * 2] * density,
                                    clip.radii[corner * 2 + 1] * density};
      OH_Drawing_RoundRectSetCorner(round_rect, kCornerPositions[corner],
                                    radii);
    }
    OH_Drawing_CanvasClipRoundRect(canvas, round_rect,
                                   OH_Drawing_CanvasClipOp::INTERSECT, true);
    OH_Drawing_RoundRectDestroy(round_rect);
  };

  replay_item_indices_.clear();
  size_t state_index = segment.initial_state_index;
  while (state_index != kInvalidDisplayListIndex &&
         state_index < segment_result.states.size()) {
    const auto& state = segment_result.states[state_index];
    replay_item_indices_.emplace_back(state.item_index);
    state_index = state.parent_state_index;
  }
  for (auto it = replay_item_indices_.rbegin();
       it != replay_item_indices_.rend(); ++it) {
    const size_t item_index = *it;
    if (item_index >= display_list.GetContentItemsSize()) {
      continue;
    }
    const auto& item = items[item_index];
    if (item.type == DisplayListOpType::kBegin) {
      apply_begin(item, item_index == 0);
    } else if (item.type == DisplayListOpType::kClipRect) {
      apply_clip_rect(item);
    }
  }

  for (size_t i = segment.start_item_index; i < segment.end_item_index; ++i) {
    const auto& item = items[i];
    switch (item.type) {
      case DisplayListOpType::kBegin:
        apply_begin(item, skip_first_translate && !has_seen_first_begin);
        break;
      case DisplayListOpType::kEnd:
        if (fragment_depth > 0) {
          OH_Drawing_CanvasRestore(canvas);
          --fragment_depth;
        }
        break;
      case DisplayListOpType::kFill: {
        const int32_t box_index = item.payload.fill.clip_index;
        if (box_index < 0 || static_cast<size_t>(box_index) >= boxes_.size()) {
          break;
        }
        if (!fill_drawable_) {
          fill_drawable_ = std::make_unique<BackgroundDrawable>(
              std::weak_ptr<UIBase>(), false);
        }
        ConfigureFillDrawable(*fill_drawable_, boxes_[box_index],
                              item.payload.fill.color, density);
        fill_drawable_->Render(canvas);
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
        apply_clip_rect(item);
        break;
      }
      case DisplayListOpType::kDrawView:
        // Consumed by SegmentDisplayList and UpdateRenderNodes.
        break;
      case DisplayListOpType::kRecordBox:
        break;
      case DisplayListOpType::kText: {
        auto text_bundle = context_->GetTextBundle(item.payload.text.text_id);
        if (text_bundle == nullptr) {
          break;
        }
        auto* paragraph = static_cast<ParagraphHarmony*>(text_bundle.get());
        paragraph->SetEmojiInvalidateTarget(renderer_host);
        paragraph->Draw(canvas, paragraph->GetTranslateLeftOffset(), 0.f);
        break;
      }
      case DisplayListOpType::kImage:
      case DisplayListOpType::kCustom:
      case DisplayListOpType::kLinearGradient:
      case DisplayListOpType::kBoxShadow:
      case DisplayListOpType::kBackgroundImage:
        // TODO: Add the remaining Harmony fragment-layer drawing operations.
        break;
      default:
        break;
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
