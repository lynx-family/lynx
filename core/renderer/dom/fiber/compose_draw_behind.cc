// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/compose_draw_behind.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/runtime/mts_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace tasm {
namespace {

// Arrays shared with the Compose producers, in physical pixels:
// [3, x, y, width, height, topLeftRadius, topRightRadius, bottomRightRadius,
//  bottomLeftRadius, offsetX, offsetY, blur, spread, ARGB]
// [4, geometry, paint] separates the shape from its coloring:
// geometry = [1, x, y, width, height] or [2, x, y, width, height, TL, TR, BR,
// BL] paint = [1, ARGB] or [2, startX, startY, endX, endY, colors[], stops[],
// 0] Gradient paint uses explicit endpoints and Clamp (0), independent of shape
// bounds.
constexpr double kCommandOutsetShadow = 3;
constexpr double kCommandPaint = 4;

struct OutsetShadow {
  float offset_x;
  float offset_y;
  float blur;
  float spread;
};

struct DrawPaint {
  bool linear_gradient{false};
  float angle{180};
  float start_x{0}, start_y{0}, end_x{0}, end_y{0};
  base::Vector<uint32_t> colors;
  base::Vector<float> stops;
};

struct DrawRect {
  double kind{0};
  RoundedRectangle clip;
  RoundedRectangle tile;
  DrawPaint paint;
  OutsetShadow shadow{};
};

bool ReadNumber(const lepus::Value& array, uint32_t index, double* result) {
  const auto value = array.GetProperty(index).ToLepusValue();
  if (!value.IsNumber() || !std::isfinite(value.Number())) return false;
  *result = value.Number();
  return true;
}

bool ReadColor(const lepus::Value& array, uint32_t index, uint32_t* result) {
  double value;
  if (!ReadNumber(array, index, &value) || value < 0 ||
      value > std::numeric_limits<uint32_t>::max() ||
      std::trunc(value) != value) {
    return false;
  }
  *result = static_cast<uint32_t>(value);
  return true;
}

bool FitsFloat(double value) {
  return std::isfinite(value) &&
         std::abs(value) <= std::numeric_limits<float>::max();
}

bool ParsePaint(const lepus::Value& value, double pixels, DrawPaint* paint) {
  if (!value.IsArrayOrJSArray()) return false;
  double kind;
  if (!ReadNumber(value, 0, &kind)) return false;
  if (kind == 1 && value.GetLength() == 2) {
    uint32_t color;
    if (!ReadColor(value, 1, &color)) return false;
    paint->colors.push_back(color);
    return true;
  }
  double mode;
  if (kind != 2 || value.GetLength() != 8 || !ReadNumber(value, 7, &mode) ||
      mode != 0)
    return false;
  float* coordinates[] = {&paint->start_x, &paint->start_y, &paint->end_x,
                          &paint->end_y};
  for (uint32_t i = 0; i < 4; ++i) {
    double coordinate;
    if (!ReadNumber(value, i + 1, &coordinate) ||
        !FitsFloat(coordinate / pixels))
      return false;
    *coordinates[i] = static_cast<float>(coordinate / pixels);
  }
  if (paint->start_x == paint->end_x && paint->start_y == paint->end_y)
    return false;
  const auto colors = value.GetProperty(5u).ToLepusValue();
  const auto stops = value.GetProperty(6u).ToLepusValue();
  if (!colors.IsArrayOrJSArray() || !stops.IsArrayOrJSArray() ||
      colors.GetLength() < 2 || colors.GetLength() != stops.GetLength())
    return false;
  double previous = 0;
  for (int i = 0; i < colors.GetLength(); ++i) {
    uint32_t color;
    double stop;
    if (!ReadColor(colors, i, &color) || !ReadNumber(stops, i, &stop) ||
        stop < previous || stop > 1)
      return false;
    paint->colors.push_back(color);
    paint->stops.push_back(static_cast<float>(stop));
    previous = stop;
  }
  paint->linear_gradient = true;
  return true;
}

// Lower explicit endpoints to the existing CSS gradient operation. The tile
// covers both the shape and endpoints; only the shape is clipped. Project the
// authored stops onto the CSS gradient line instead of stretching to the shape.
bool LowerLinearPaint(DrawRect* rect) {
  if (rect->clip.GetWidth() == 0 || rect->clip.GetHeight() == 0) return true;
  auto& paint = rect->paint;
  const double left = std::min({rect->clip.GetX(), paint.start_x, paint.end_x});
  const double top = std::min({rect->clip.GetY(), paint.start_y, paint.end_y});
  const double right = std::max(
      {static_cast<double>(rect->clip.GetX()) + rect->clip.GetWidth(),
       static_cast<double>(paint.start_x), static_cast<double>(paint.end_x)});
  const double bottom = std::max(
      {static_cast<double>(rect->clip.GetY()) + rect->clip.GetHeight(),
       static_cast<double>(paint.start_y), static_cast<double>(paint.end_y)});
  if (!FitsFloat(right - left) || !FitsFloat(bottom - top)) return false;
  rect->tile.SetX(static_cast<float>(left));
  rect->tile.SetY(static_cast<float>(top));
  rect->tile.SetWidth(static_cast<float>(right - left));
  rect->tile.SetHeight(static_cast<float>(bottom - top));

  const double dx = static_cast<double>(paint.end_x) - paint.start_x;
  const double dy = static_cast<double>(paint.end_y) - paint.start_y;
  const double degrees = 180.0 / std::acos(-1.0);
  double angle = std::atan2(dx, -dy) * degrees;
  if (angle < 0) angle += 360;
  paint.angle = static_cast<float>(angle);
  // Use the recorded angle and dimensions, including their float rounding.
  const double direction_x = std::sin(paint.angle / degrees);
  const double direction_y = -std::cos(paint.angle / degrees);
  const double width = rect->tile.GetWidth();
  const double height = rect->tile.GetHeight();
  const double span =
      std::abs(width * direction_x) + std::abs(height * direction_y);
  if (span <= 0) return false;
  const double center_x = left + width / 2;
  const double center_y = top + height / 2;
  const double start = ((paint.start_x - center_x) * direction_x +
                        (paint.start_y - center_y) * direction_y + span / 2) /
                       span;
  const double scale = (dx * direction_x + dy * direction_y) / span;
  base::Vector<uint32_t> colors;
  base::Vector<float> stops;
  // Explicit endpoint colors preserve Clamp outside the authored interval.
  colors.push_back(paint.colors.front());
  stops.push_back(0);
  for (size_t i = 0; i < paint.colors.size(); ++i) {
    colors.push_back(paint.colors[i]);
    stops.push_back(static_cast<float>(
        std::clamp(start + paint.stops[i] * scale, 0.0, 1.0)));
  }
  colors.push_back(paint.colors.back());
  stops.push_back(1);
  paint.colors = std::move(colors);
  paint.stops = std::move(stops);
  return true;
}

bool ParsePaintCommand(const lepus::Value& value, double pixels,
                       DrawRect* rect) {
  if (value.GetLength() != 3) return false;
  const auto geometry = value.GetProperty(1u).ToLepusValue();
  if (!geometry.IsArrayOrJSArray()) return false;
  double shape;
  if (!ReadNumber(geometry, 0, &shape) ||
      !((shape == 1 && geometry.GetLength() == 5) ||
        (shape == 2 && geometry.GetLength() == 9)))
    return false;
  double bounds[4];
  for (uint32_t i = 0; i < 4; ++i) {
    if (!ReadNumber(geometry, i + 1, &bounds[i])) return false;
    bounds[i] /= pixels;
    if (!FitsFloat(bounds[i])) return false;
  }
  if (bounds[2] < 0 || bounds[3] < 0 || !FitsFloat(bounds[0] + bounds[2]) ||
      !FitsFloat(bounds[1] + bounds[3]))
    return false;
  rect->clip.SetX(static_cast<float>(bounds[0]));
  rect->clip.SetY(static_cast<float>(bounds[1]));
  rect->clip.SetWidth(static_cast<float>(bounds[2]));
  rect->clip.SetHeight(static_cast<float>(bounds[3]));
  if (shape == 2) {
    float radii[4];
    for (uint32_t i = 0; i < 4; ++i) {
      double radius;
      if (!ReadNumber(geometry, i + 5, &radius) || radius < 0 ||
          !FitsFloat(radius / pixels))
        return false;
      radii[i] = static_cast<float>(radius / pixels);
    }
    rect->clip.SetRadiusXTopLeft(radii[0]);
    rect->clip.SetRadiusYTopLeft(radii[0]);
    rect->clip.SetRadiusXTopRight(radii[1]);
    rect->clip.SetRadiusYTopRight(radii[1]);
    rect->clip.SetRadiusXBottomRight(radii[2]);
    rect->clip.SetRadiusYBottomRight(radii[2]);
    rect->clip.SetRadiusXBottomLeft(radii[3]);
    rect->clip.SetRadiusYBottomLeft(radii[3]);
  }
  if (!ParsePaint(value.GetProperty(2u).ToLepusValue(), pixels, &rect->paint))
    return false;
  return !rect->paint.linear_gradient || LowerLinearPaint(rect);
}

bool ParseRect(const lepus::Value& value, double pixels, DrawRect* rect) {
  if (!value.IsArrayOrJSArray()) return false;
  const int length = value.GetLength();
  if (!ReadNumber(value, 0, &rect->kind)) return false;
  const double kind = rect->kind;
  if (kind == kCommandPaint) return ParsePaintCommand(value, pixels, rect);
  if (kind != kCommandOutsetShadow || length != 14) return false;
  double bounds[4];
  for (uint32_t i = 0; i < 4; ++i) {
    if (!ReadNumber(value, i + 1, &bounds[i])) return false;
    bounds[i] /= pixels;
    if (!FitsFloat(bounds[i])) return false;
  }
  if (bounds[2] < 0 || bounds[3] < 0 || !FitsFloat(bounds[0] + bounds[2]) ||
      !FitsFloat(bounds[1] + bounds[3]))
    return false;
  rect->clip.SetX(static_cast<float>(bounds[0]));
  rect->clip.SetY(static_cast<float>(bounds[1]));
  rect->clip.SetWidth(static_cast<float>(bounds[2]));
  rect->clip.SetHeight(static_cast<float>(bounds[3]));
  double radii[4];
  for (uint32_t i = 0; i < 4; ++i) {
    if (!ReadNumber(value, i + 5, &radii[i]) || radii[i] < 0) return false;
    radii[i] /= pixels;
    if (!FitsFloat(radii[i])) return false;
  }
  const auto r = [&radii](int i) { return static_cast<float>(radii[i]); };
  rect->clip.SetRadiusXTopLeft(r(0));
  rect->clip.SetRadiusYTopLeft(r(0));
  rect->clip.SetRadiusXTopRight(r(1));
  rect->clip.SetRadiusYTopRight(r(1));
  rect->clip.SetRadiusXBottomRight(r(2));
  rect->clip.SetRadiusYBottomRight(r(2));
  rect->clip.SetRadiusXBottomLeft(r(3));
  rect->clip.SetRadiusYBottomLeft(r(3));
  double shadow[4];
  for (uint32_t i = 0; i < 4; ++i) {
    if (!ReadNumber(value, i + 9, &shadow[i])) return false;
    shadow[i] /= pixels;
    if (!FitsFloat(shadow[i])) return false;
  }
  if (shadow[2] < 0) return false;
  rect->shadow = {static_cast<float>(shadow[0]), static_cast<float>(shadow[1]),
                  static_cast<float>(shadow[2]), static_cast<float>(shadow[3])};
  uint32_t color;
  if (!ReadColor(value, 13, &color)) return false;
  rect->paint.colors.push_back(color);
  return true;
}

CustomDrawFunctions ComposeDrawFunctions(runtime::MTSRuntime* runtime,
                                         lepus::Value callback,
                                         bool foreground) {
  CustomDrawFunctions functions;
  auto draw = [runtime, callback](Fragment& fragment,
                                  DisplayListBuilder& builder) {
    auto* target = fragment.element();
    if (!target->IsAttached() || !target->HasCustomDrawFunctions() ||
        (target->dirty() & Element::kDirtyCustomDraw))
      return;
    const double pixels = target->element_manager()
                              ->GetLynxEnvConfig()
                              .PhysicalPixelsPerLayoutUnit();
    if (!std::isfinite(pixels) || pixels <= 0) return;
    const lepus::Value element{fml::RefPtr<Element>(target)};
    const lepus::Value width(target->width() * pixels);
    const lepus::Value height(target->height() * pixels);
    const lepus::Value* args[] = {&element, &width, &height};
    const auto value = runtime->GetMTSContext()
                           ->CallClosureArgs(callback, args, 3)
                           .ToLepusValue();
    if (value.IsEmpty()) return;
    if (!value.IsArrayOrJSArray()) {
      LOGE("Compose draw resolver returned invalid commands");
      return;
    }
    // Validate the entire result before recording, so malformed commands never
    // leave partially drawn content in the new DisplayList.
    std::vector<DrawRect> commands;
    for (int i = 0; i < value.GetLength(); ++i) {
      DrawRect rect;
      if (!ParseRect(value.GetProperty(static_cast<uint32_t>(i)).ToLepusValue(),
                     pixels, &rect)) {
        LOGE("Compose draw resolver returned an invalid rectangle");
        return;
      }
      commands.push_back(std::move(rect));
    }
    for (const auto& rect : commands) {
      if (rect.clip.GetWidth() == 0 || rect.clip.GetHeight() == 0) continue;
      if (rect.kind == kCommandOutsetShadow) {
        fragment.DrawCustomOutsetShadow(
            builder, rect.clip, rect.shadow.offset_x, rect.shadow.offset_y,
            rect.shadow.blur, rect.shadow.spread, rect.paint.colors[0]);
        continue;
      }
      int32_t clip_index;
      builder.RecordBoxModel(rect.clip, clip_index);
      if (!rect.paint.linear_gradient) {
        builder.Fill(rect.paint.colors[0], clip_index);
      } else {
        int32_t tile_index;
        builder.RecordBoxModel(rect.tile, tile_index);
        const auto no_repeat =
            static_cast<int32_t>(starlight::BackgroundRepeatType::kNoRepeat);
        builder.LinearGradient(rect.paint.angle, rect.paint.colors,
                               rect.paint.stops, tile_index, clip_index,
                               no_repeat, no_repeat);
      }
    }
  };
  if (foreground) {
    functions.draw_foreground = std::move(draw);
  } else {
    functions.draw_behind = std::move(draw);
  }
  return functions;
}

}  // namespace

CustomDrawFunctions ComposeDrawBehindFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback) {
  return ComposeDrawFunctions(runtime, std::move(callback), false);
}

CustomDrawFunctions ComposeDrawForegroundFunctions(runtime::MTSRuntime* runtime,
                                                   lepus::Value callback) {
  return ComposeDrawFunctions(runtime, std::move(callback), true);
}

}  // namespace tasm
}  // namespace lynx
