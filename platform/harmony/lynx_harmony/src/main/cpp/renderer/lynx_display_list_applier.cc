// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_display_list_applier.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include "base/include/value/array.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/shader_effect.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"
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

void ClipRoundedRect(OH_Drawing_Canvas* canvas, OH_Drawing_Rect* rect,
                     const float* radii, float density) {
  auto* round_rect = OH_Drawing_RoundRectCreate(rect, 0.f, 0.f);
  constexpr OH_Drawing_CornerPos kCornerPositions[] = {
      CORNER_POS_TOP_LEFT, CORNER_POS_TOP_RIGHT, CORNER_POS_BOTTOM_RIGHT,
      CORNER_POS_BOTTOM_LEFT};
  for (size_t corner = 0; corner < 4; ++corner) {
    OH_Drawing_Corner_Radii corner_radii{radii[corner * 2] * density,
                                         radii[corner * 2 + 1] * density};
    OH_Drawing_RoundRectSetCorner(round_rect, kCornerPositions[corner],
                                  corner_radii);
  }
  OH_Drawing_CanvasClipRoundRect(canvas, round_rect,
                                 OH_Drawing_CanvasClipOp::INTERSECT, true);
  OH_Drawing_RoundRectDestroy(round_rect);
}

bool ResolveGradientData(const DisplayListItem& item,
                         const DisplayListReader& reader, uint32_t color_count,
                         uint32_t stop_count, const uint32_t*& colors,
                         const float*& stops,
                         std::vector<float>& default_stops) {
  if (color_count < 2 || (stop_count != 0 && stop_count != color_count)) {
    return false;
  }
  colors = reader.Colors(item);
  if (colors == nullptr) {
    return false;
  }
  if (stop_count != 0) {
    stops = reader.Stops(item);
    return stops != nullptr;
  }

  default_stops.resize(color_count);
  const float interval = 1.f / static_cast<float>(color_count - 1);
  for (uint32_t i = 0; i < color_count; ++i) {
    default_stops[i] = i == color_count - 1 ? 1.f : i * interval;
  }
  stops = default_stops.data();
  return true;
}

void ClipToBox(OH_Drawing_Canvas* canvas, const RoundedRectangle& clip_box,
               float density) {
  auto* clip_rect = OH_Drawing_RectCreate(
      clip_box.GetX() * density, clip_box.GetY() * density,
      (clip_box.GetX() + clip_box.GetWidth()) * density,
      (clip_box.GetY() + clip_box.GetHeight()) * density);
  if (clip_box.HasRadius()) {
    const float radii[] = {
        clip_box.GetRadiusXTopLeft(),     clip_box.GetRadiusYTopLeft(),
        clip_box.GetRadiusXTopRight(),    clip_box.GetRadiusYTopRight(),
        clip_box.GetRadiusXBottomRight(), clip_box.GetRadiusYBottomRight(),
        clip_box.GetRadiusXBottomLeft(),  clip_box.GetRadiusYBottomLeft()};
    ClipRoundedRect(canvas, clip_rect, radii, density);
  } else {
    OH_Drawing_CanvasClipRect(canvas, clip_rect,
                              OH_Drawing_CanvasClipOp::INTERSECT, true);
  }
  OH_Drawing_RectDestroy(clip_rect);
}

bool HasGradientArea(const RoundedRectangle& tiling_box,
                     const RoundedRectangle& clip_box) {
  return tiling_box.GetWidth() > 0.f && tiling_box.GetHeight() > 0.f &&
         clip_box.GetWidth() > 0.f && clip_box.GetHeight() > 0.f;
}

template <typename DrawTile>
void DrawTiled(OH_Drawing_Canvas* canvas, const RoundedRectangle& tiling_box,
               const RoundedRectangle& clip_box, int32_t repeat_x,
               int32_t repeat_y, float density, const DrawTile& draw_tile) {
  const float width = tiling_box.GetWidth() * density;
  const float height = tiling_box.GetHeight() * density;
  const float clip_left = clip_box.GetX() * density;
  const float clip_top = clip_box.GetY() * density;
  const float clip_right = (clip_box.GetX() + clip_box.GetWidth()) * density;
  const float clip_bottom = (clip_box.GetY() + clip_box.GetHeight()) * density;
  const double origin_x = tiling_box.GetX() * density;
  const double origin_y = tiling_box.GetY() * density;

  const bool repeat_horizontally =
      static_cast<starlight::BackgroundRepeatType>(repeat_x) ==
      starlight::BackgroundRepeatType::kRepeat;
  const bool repeat_vertically =
      static_cast<starlight::BackgroundRepeatType>(repeat_y) ==
      starlight::BackgroundRepeatType::kRepeat;
  const float tile_width =
      repeat_horizontally ? std::max(1.f, std::round(width)) : width;
  const float tile_height =
      repeat_vertically ? std::max(1.f, std::round(height)) : height;
  // Align repeated tiles in drawing pixels and include the partially visible
  // first tile even when the clip starts at a fractional coordinate.
  auto first_tile = [](double origin, double clip_start, double size) {
    double phase = std::fmod(clip_start - origin, size);
    if (phase < 0.0) {
      phase += size;
    }
    return clip_start - phase;
  };
  const double start_x =
      repeat_horizontally
          ? first_tile(std::round(origin_x), std::floor(clip_left), tile_width)
          : origin_x;
  const double start_y =
      repeat_vertically
          ? first_tile(std::round(origin_y), std::floor(clip_top), tile_height)
          : origin_y;
  const double columns = repeat_horizontally
                             ? std::ceil((clip_right - start_x) / tile_width)
                             : 1.0;
  const double rows = repeat_vertically
                          ? std::ceil((clip_bottom - start_y) / tile_height)
                          : 1.0;
  const double max_count =
      static_cast<double>(std::numeric_limits<size_t>::max());
  if (!(columns > 0.0 && columns < max_count && rows > 0.0 &&
        rows < max_count)) {
    return;
  }
  const size_t column_count = static_cast<size_t>(columns);
  const size_t row_count = static_cast<size_t>(rows);

  OH_Drawing_CanvasSave(canvas);
  ClipToBox(canvas, clip_box, density);
  // Count tiles with integers; floating-point += can stop making progress.
  // Translate from the saved canvas each time to avoid accumulated rounding.
  for (size_t column = 0; column < column_count; ++column) {
    for (size_t row = 0; row < row_count; ++row) {
      OH_Drawing_CanvasSave(canvas);
      OH_Drawing_CanvasTranslate(
          canvas,
          static_cast<float>(start_x +
                             column * static_cast<double>(tile_width)),
          static_cast<float>(start_y + row * static_cast<double>(tile_height)));
      draw_tile(tile_width, tile_height);
      OH_Drawing_CanvasRestore(canvas);
    }
  }
  OH_Drawing_CanvasRestore(canvas);
}

void DrawGradient(OH_Drawing_Canvas* canvas, OH_Drawing_ShaderEffect* shader,
                  const RoundedRectangle& tiling_box,
                  const RoundedRectangle& clip_box, int32_t repeat_x,
                  int32_t repeat_y, float density) {
  if (shader == nullptr) {
    return;
  }
  // Both gradient entry points validate geometry before creating the shader.
  const float width = tiling_box.GetWidth() * density;
  const float height = tiling_box.GetHeight() * density;
  auto* brush = OH_Drawing_BrushCreate();
  auto* tile_rect = OH_Drawing_RectCreate(0.f, 0.f, width, height);
  // Adjacent tiles must not blend their shared edges with the background.
  // The outer rounded clip still provides anti-aliasing.
  OH_Drawing_BrushSetAntiAlias(
      brush, static_cast<starlight::BackgroundRepeatType>(repeat_x) !=
                     starlight::BackgroundRepeatType::kRepeat &&
                 static_cast<starlight::BackgroundRepeatType>(repeat_y) !=
                     starlight::BackgroundRepeatType::kRepeat);
  OH_Drawing_BrushSetShaderEffect(brush, shader);
  OH_Drawing_CanvasAttachBrush(canvas, brush);
  DrawTiled(canvas, tiling_box, clip_box, repeat_x, repeat_y, density,
            [&](float tile_width, float tile_height) {
              // Scale the shader and rectangle together to fit the aligned
              // tile.
              OH_Drawing_CanvasScale(canvas, tile_width / width,
                                     tile_height / height);
              OH_Drawing_CanvasDrawRect(canvas, tile_rect);
            });
  OH_Drawing_CanvasDetachBrush(canvas);
  OH_Drawing_RectDestroy(tile_rect);
  OH_Drawing_BrushDestroy(brush);
}

void DrawLinearGradient(OH_Drawing_Canvas* canvas, const DisplayListItem& item,
                        const DisplayListReader& reader,
                        const std::vector<RoundedRectangle>& boxes,
                        float density) {
  const auto& gradient = item.payload.linear_gradient;
  if (gradient.tiling_index < 0 || gradient.clip_index < 0 ||
      static_cast<size_t>(gradient.tiling_index) >= boxes.size() ||
      static_cast<size_t>(gradient.clip_index) >= boxes.size()) {
    return;
  }
  const uint32_t* colors = nullptr;
  const float* stops = nullptr;
  std::vector<float> default_stops;
  if (!ResolveGradientData(item, reader, gradient.color_count,
                           gradient.stop_count, colors, stops, default_stops)) {
    return;
  }

  const auto& tiling_box = boxes[gradient.tiling_index];
  if (!HasGradientArea(tiling_box, boxes[gradient.clip_index])) {
    return;
  }
  const float width = tiling_box.GetWidth() * density;
  const float height = tiling_box.GetHeight() * density;
  const float radians = gradient.angle * M_PI / 180.0;
  const float direction_x = std::sin(radians);
  const float direction_y = -std::cos(radians);
  const float line_length =
      std::abs(width * direction_x) + std::abs(height * direction_y);
  const float center_x = width / 2.f;
  const float center_y = height / 2.f;
  const float offset_x = direction_x * line_length / 2.f;
  const float offset_y = direction_y * line_length / 2.f;
  auto shader = ShaderEffect::CreateLinearGradientEffect(
      center_x - offset_x, center_y - offset_y, center_x + offset_x,
      center_y + offset_y, colors, stops, gradient.color_count, CLAMP);
  DrawGradient(canvas, shader->HarmonyShaderEffect(), tiling_box,
               boxes[gradient.clip_index], gradient.repeat_x, gradient.repeat_y,
               density);
}

void DrawRadialGradient(OH_Drawing_Canvas* canvas, const DisplayListItem& item,
                        const DisplayListReader& reader,
                        const std::vector<RoundedRectangle>& boxes,
                        float density) {
  const auto& gradient = item.payload.radial_gradient;
  if (gradient.tiling_index < 0 || gradient.clip_index < 0 ||
      static_cast<size_t>(gradient.tiling_index) >= boxes.size() ||
      static_cast<size_t>(gradient.clip_index) >= boxes.size()) {
    return;
  }
  if (!HasGradientArea(boxes[gradient.tiling_index],
                       boxes[gradient.clip_index])) {
    return;
  }
  const uint32_t* colors = nullptr;
  const float* stops = nullptr;
  std::vector<float> default_stops;
  if (!ResolveGradientData(item, reader, gradient.color_count,
                           gradient.stop_count, colors, stops, default_stops)) {
    return;
  }

  const float center_x = gradient.center_x * density;
  const float center_y = gradient.center_y * density;
  const float radius_x = gradient.radius_x * density;
  const float radius_y = gradient.radius_y * density;
  if (radius_x < 0.f || radius_y < 0.f) {
    return;
  }
  if (radius_x == 0.f || radius_y == 0.f) {
    // Stops in the display list are relative positions. With a zero radius,
    // their physical positions collapse to the center; use the final color.
    const uint32_t solid_colors[] = {colors[gradient.color_count - 1],
                                     colors[gradient.color_count - 1]};
    const float solid_stops[] = {0.f, 1.f};
    auto shader = ShaderEffect::CreateLinearGradientEffect(
        0.f, 0.f, 1.f, 0.f, solid_colors, solid_stops, 2, CLAMP);
    DrawGradient(canvas, shader->HarmonyShaderEffect(),
                 boxes[gradient.tiling_index], boxes[gradient.clip_index],
                 gradient.repeat_x, gradient.repeat_y, density);
    return;
  }
  OH_Drawing_Matrix* matrix = nullptr;
  const float radius = std::max(radius_x, radius_y);
  if (radius_x != radius_y) {
    matrix = OH_Drawing_MatrixCreate();
    OH_Drawing_MatrixScale(matrix, radius_x / radius, radius_y / radius,
                           center_x, center_y);
  }
  auto shader = ShaderEffect::CreateRadialGradientEffect(
      center_x, center_y, radius, colors, stops, gradient.color_count, CLAMP,
      matrix);
  DrawGradient(canvas, shader->HarmonyShaderEffect(),
               boxes[gradient.tiling_index], boxes[gradient.clip_index],
               gradient.repeat_x, gradient.repeat_y, density);
}

base::Vector<DisplayListSegment> SegmentDisplayList(
    const DisplayList& display_list, std::vector<RoundedRectangle>& boxes) {
  boxes.clear();
  base::Vector<DisplayListSegment> segments;
  const size_t item_count = display_list.GetContentItemsSize();
  if (item_count == 0) {
    return segments;
  }

  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  DisplayListSegment segment;
  base::Vector<size_t> state;
  base::Vector<size_t> scopes;
  for (size_t i = 0; i < item_count; ++i) {
    switch (items[i].type) {
      case DisplayListOpType::kBegin:
        scopes.push_back(state.size());
        state.push_back(i);
        break;
      case DisplayListOpType::kEnd:
        if (!scopes.empty()) {
          state.resize<false>(scopes.back());
          scopes.pop_back();
        }
        break;
      case DisplayListOpType::kClipRect:
        state.push_back(i);
        break;
      case DisplayListOpType::kRecordBox:
        boxes.emplace_back(RecordBox(items[i]));
        break;
      case DisplayListOpType::kDrawView:
        segment.end_item_index = i;
        segments.push_back(std::move(segment));
        segment = DisplayListSegment{};
        segment.start_item_index = i + 1;
        segment.preceding_view_id = items[i].payload.draw_view.view_id;
        segment.state_item_indices = state;
        break;
      default:
        segment.has_drawing = true;
        break;
    }
  }
  segment.end_item_index = item_count;
  segments.push_back(std::move(segment));
  return segments;
}
}  // namespace

LynxDisplayListApplier::LynxDisplayListApplier(LynxRendererContext* context,
                                               std::weak_ptr<UIBase> host)
    : context_(context), host_(std::move(host)) {}

LynxDisplayListApplier::~LynxDisplayListApplier() = default;

base::Vector<DisplayListSegment> LynxDisplayListApplier::UpdateDisplayList(
    const DisplayList& display_list) {
  return SegmentDisplayList(display_list, boxes_);
}

void LynxDisplayListApplier::ApplyDisplayList(
    const DisplayList& display_list, OH_Drawing_Canvas* canvas,
    const DisplayListSegment& segment) {
  if (context_ == nullptr || canvas == nullptr) {
    return;
  }
  auto lynx_context = context_->GetLynxContext();
  if (lynx_context == nullptr) {
    return;
  }

  const size_t item_count = display_list.GetContentItemsSize();
  const auto* items = reinterpret_cast<const DisplayListItem*>(
      display_list.GetContentItemsData());
  if (item_count == 0 || items == nullptr) {
    return;
  }
  DisplayListReader reader(display_list);
  ProcessContentOperations(items, reader, canvas, lynx_context->ScaledDensity(),
                           segment);
}

void LynxDisplayListApplier::ProcessContentOperations(
    const DisplayListItem* items, const DisplayListReader& reader,
    OH_Drawing_Canvas* canvas, float density,
    const DisplayListSegment& segment) {
  int32_t fragment_depth = 0;
  bool has_seen_first_begin = false;

  OH_Drawing_CanvasSave(canvas);
  const size_t state_count = segment.state_item_indices.size();
  for (size_t cursor = 0; cursor < state_count + segment.ItemCount();
       ++cursor) {
    const size_t i = cursor < state_count
                         ? segment.state_item_indices[cursor]
                         : segment.start_item_index + cursor - state_count;
    const auto& item = items[i];
    switch (item.type) {
      case DisplayListOpType::kBegin:
        OH_Drawing_CanvasSave(canvas);
        if (has_seen_first_begin) {
          OH_Drawing_CanvasTranslate(canvas, item.payload.begin.x * density,
                                     item.payload.begin.y * density);
        }
        has_seen_first_begin = true;
        ++fragment_depth;
        break;
      case DisplayListOpType::kEnd:
        if (fragment_depth > 0) {
          OH_Drawing_CanvasRestore(canvas);
          --fragment_depth;
        }
        break;
      case DisplayListOpType::kRecordBox:
        break;
      case DisplayListOpType::kFill: {
        const int32_t box_index = item.payload.fill.clip_index;
        if (box_index < 0 || static_cast<size_t>(box_index) >= boxes_.size()) {
          break;
        }
        if (!fill_drawable_) {
          fill_drawable_ = std::make_unique<BackgroundDrawable>(host_, false);
        }
        ConfigureFillDrawable(*fill_drawable_, boxes_[box_index],
                              item.payload.fill.color, density);
        fill_drawable_->RenderForFragmentLayer(
            canvas, BackgroundDrawable::FragmentLayerRenderMode::kBackground);
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
          border_drawable_ = std::make_unique<BackgroundDrawable>(host_, false);
        }
        ConfigureBorderDrawable(*border_drawable_, boxes_[outer_index],
                                boxes_[inner_index], item.payload, density);
        border_drawable_->RenderForFragmentLayer(
            canvas, BackgroundDrawable::FragmentLayerRenderMode::kBorder);
        break;
      }
      case DisplayListOpType::kClipRect: {
        const auto& clip = item.payload.clip_rect;
        auto* rect = OH_Drawing_RectCreate(clip.x * density, clip.y * density,
                                           (clip.x + clip.w) * density,
                                           (clip.y + clip.h) * density);
        if (!clip.has_radii) {
          OH_Drawing_CanvasClipRect(canvas, rect,
                                    OH_Drawing_CanvasClipOp::INTERSECT, true);
          OH_Drawing_RectDestroy(rect);
          break;
        }
        ClipRoundedRect(canvas, rect, clip.radii, density);
        OH_Drawing_RectDestroy(rect);
        break;
      }
      case DisplayListOpType::kText: {
        auto text_bundle = context_->GetTextBundle(item.payload.text.text_id);
        if (text_bundle != nullptr) {
          text_bundle->SetEmojiInvalidateTarget(host_);
          text_bundle->Draw(canvas, text_bundle->GetTranslateLeftOffset(), 0.f);
        }
        break;
      }
      case DisplayListOpType::kImage: {
        const int32_t image_id = item.payload.image.image_id;
        const int32_t box_index = item.payload.image.box_index;
        if (box_index < 0 || static_cast<size_t>(box_index) >= boxes_.size()) {
          break;
        }
        auto host = host_.lock();
        if (host == nullptr) {
          break;
        }
        auto image_manager = context_->GetImageManager(image_id);
        const auto& box = boxes_[box_index];
        if (image_manager == nullptr || box.GetWidth() <= 0.f ||
            box.GetHeight() <= 0.f) {
          break;
        }

        image_manager->SetTarget(host);
        OH_Drawing_CanvasSave(canvas);
        ClipToBox(canvas, box, density);
        OH_Drawing_CanvasTranslate(canvas, box.GetX() * density,
                                   box.GetY() * density);
        image_manager->UpdateBounds(box.GetWidth(), box.GetHeight(), density);
        image_manager->Draw(canvas);
        OH_Drawing_CanvasRestore(canvas);
        break;
      }
      case DisplayListOpType::kDrawView:
        // Native views separate the segments composed by LynxRenderer.
        break;
      case DisplayListOpType::kCustom:
      case DisplayListOpType::kBoxShadow:
        // TODO: Add the remaining Harmony fragment-layer drawing operations.
        break;
      case DisplayListOpType::kLinearGradient:
        DrawLinearGradient(canvas, item, reader, boxes_, density);
        break;
      case DisplayListOpType::kRadialGradient:
        DrawRadialGradient(canvas, item, reader, boxes_, density);
        break;
      case DisplayListOpType::kBackgroundImage: {
        const auto& background = item.payload.background_image;
        DrawBackgroundImage(canvas, background.image_id,
                            background.tiling_index, background.clip_index,
                            background.repeat_x, background.repeat_y,
                            background.auto_size, background.position_x,
                            background.position_y, density);
        break;
      }
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

void LynxDisplayListApplier::DrawBackgroundImage(
    OH_Drawing_Canvas* canvas, int32_t image_id, int32_t tiling_index,
    int32_t clip_index, int32_t repeat_x, int32_t repeat_y, int32_t auto_size,
    float position_x, float position_y, float density) {
  if (tiling_index < 0 || clip_index < 0 ||
      static_cast<size_t>(tiling_index) >= boxes_.size() ||
      static_cast<size_t>(clip_index) >= boxes_.size() || density <= 0.f) {
    return;
  }
  auto host = host_.lock();
  auto image_manager = context_->GetImageManager(image_id);
  if (host == nullptr || image_manager == nullptr) {
    return;
  }

  // Bind before deferring an unloaded image so completion schedules a redraw.
  image_manager->SetTarget(host);
  auto tiling_box = boxes_[tiling_index];
  if (auto_size != 0) {
    const float image_width = image_manager->GetImageWidth();
    const float image_height = image_manager->GetImageHeight();
    if (image_width <= 0.f || image_height <= 0.f) {
      return;
    }
    float width = tiling_box.GetWidth();
    float height = tiling_box.GetHeight();
    const bool auto_width = (auto_size & 1) != 0;
    const bool auto_height = (auto_size & 2) != 0;
    if (auto_width && auto_height) {
      // Boxes use logical units; DrawTiled applies density once below.
      width = image_width;
      height = image_height;
    } else if (auto_width) {
      width = height * image_width / image_height;
    } else if (auto_height) {
      height = width * image_height / image_width;
    }
    tiling_box.SetX(tiling_box.GetX() +
                    (tiling_box.GetWidth() - width) * position_x);
    tiling_box.SetY(tiling_box.GetY() +
                    (tiling_box.GetHeight() - height) * position_y);
    tiling_box.SetWidth(width);
    tiling_box.SetHeight(height);
  }
  const auto& clip_box = boxes_[clip_index];
  if (tiling_box.GetWidth() <= 0.f || tiling_box.GetHeight() <= 0.f ||
      clip_box.GetWidth() <= 0.f || clip_box.GetHeight() <= 0.f) {
    return;
  }
  DrawTiled(canvas, tiling_box, clip_box, repeat_x, repeat_y, density,
            [&](float tile_width, float tile_height) {
              image_manager->UpdateBounds(tile_width / density,
                                          tile_height / density, density);
              image_manager->Draw(canvas);
            });
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
