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
// [1, x, y, width, height, ARGB]
// [2, x, y, width, height, startY, endY, colors[], stops[]]
// [3, x, y, width, height, topLeftRadius, topRightRadius, bottomRightRadius,
//  bottomLeftRadius, offsetX, offsetY, blur, spread, ARGB]
// Filled rectangles, clamped vertical gradients and outset shadows of a
// rounded rectangle are supported here.
constexpr double kCommandFill = 1;
constexpr double kCommandGradient = 2;
constexpr double kCommandOutsetShadow = 3;

struct OutsetShadow {
  float offset_x;
  float offset_y;
  float blur;
  float spread;
};

struct DrawRect {
  double kind{0};
  RoundedRectangle clip;
  RoundedRectangle tile;
  base::Vector<uint32_t> colors;
  base::Vector<float> stops;
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

bool ParseRect(const lepus::Value& value, double pixels, DrawRect* rect) {
  if (!value.IsArrayOrJSArray()) return false;
  const int length = value.GetLength();
  if (!ReadNumber(value, 0, &rect->kind)) return false;
  const double kind = rect->kind;
  if (!((kind == kCommandFill && length == 6) ||
        (kind == kCommandGradient && length == 9) ||
        (kind == kCommandOutsetShadow && length == 14))) {
    return false;
  }
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
  if (kind == kCommandFill) {
    uint32_t color;
    if (!ReadColor(value, 5, &color)) return false;
    rect->colors.push_back(color);
    return true;
  }
  if (kind == kCommandOutsetShadow) {
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
    rect->shadow = {
        static_cast<float>(shadow[0]), static_cast<float>(shadow[1]),
        static_cast<float>(shadow[2]), static_cast<float>(shadow[3])};
    uint32_t color;
    if (!ReadColor(value, 13, &color)) return false;
    rect->colors.push_back(color);
    return true;
  }
  double start, end;
  if (!ReadNumber(value, 5, &start) || !ReadNumber(value, 6, &end))
    return false;
  start /= pixels;
  end /= pixels;
  if (!FitsFloat(start) || !FitsFloat(end) || start >= end) return false;
  const auto colors = value.GetProperty(7u).ToLepusValue();
  const auto stops = value.GetProperty(8u).ToLepusValue();
  if (!colors.IsArrayOrJSArray() || !stops.IsArrayOrJSArray() ||
      colors.GetLength() < 2 || colors.GetLength() != stops.GetLength()) {
    return false;
  }

  // Expand the gradient tile to cover the draw rect, preserving the shader's
  // coordinates. Explicit endpoint colors implement Clamp without painting
  // translucent pixels twice or stretching the gradient to the clipped rect.
  const double top = std::min(bounds[1], start);
  const double height = std::max(bounds[1] + bounds[3], end) - top;
  if (!FitsFloat(height) || static_cast<float>(height) <= 0) return false;
  rect->tile = rect->clip;
  rect->tile.SetY(static_cast<float>(top));
  rect->tile.SetHeight(static_cast<float>(height));
  double previous = 0;
  for (int i = 0; i < colors.GetLength(); ++i) {
    uint32_t color;
    double stop;
    if (!ReadColor(colors, i, &color) || !ReadNumber(stops, i, &stop) ||
        stop < previous || stop > 1)
      return false;
    if (i == 0) {
      rect->colors.push_back(color);
      rect->stops.push_back(0);
    }
    rect->colors.push_back(color);
    rect->stops.push_back(
        static_cast<float>((start + stop * (end - start) - top) / height));
    previous = stop;
  }
  rect->colors.push_back(rect->colors.back());
  rect->stops.push_back(1);
  return true;
}

}  // namespace

CustomDrawFunctions ComposeDrawBehindFunctions(runtime::MTSRuntime* runtime,
                                               lepus::Value callback) {
  CustomDrawFunctions functions;
  functions.draw_behind = [runtime, callback](Fragment& fragment,
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
      LOGE("drawBehind resolver returned invalid commands");
      return;
    }
    // Validate the entire result before recording, so malformed commands never
    // leave a partially drawn background in the new DisplayList.
    std::vector<DrawRect> commands;
    for (int i = 0; i < value.GetLength(); ++i) {
      DrawRect rect;
      if (!ParseRect(value.GetProperty(static_cast<uint32_t>(i)).ToLepusValue(),
                     pixels, &rect)) {
        LOGE("drawBehind resolver returned an invalid rectangle");
        return;
      }
      commands.push_back(std::move(rect));
    }
    for (const auto& rect : commands) {
      if (rect.clip.GetWidth() == 0 || rect.clip.GetHeight() == 0) continue;
      if (rect.kind == kCommandOutsetShadow) {
        fragment.DrawCustomOutsetShadow(
            builder, rect.clip, rect.shadow.offset_x, rect.shadow.offset_y,
            rect.shadow.blur, rect.shadow.spread, rect.colors[0]);
        continue;
      }
      int32_t clip_index;
      builder.RecordBoxModel(rect.clip, clip_index);
      if (rect.kind == kCommandFill) {
        builder.Fill(rect.colors[0], clip_index);
      } else {
        int32_t tile_index;
        builder.RecordBoxModel(rect.tile, tile_index);
        const auto no_repeat =
            static_cast<int32_t>(starlight::BackgroundRepeatType::kNoRepeat);
        builder.LinearGradient(180, rect.colors, rect.stops, tile_index,
                               clip_index, no_repeat, no_repeat);
      }
    }
  };
  return functions;
}

}  // namespace tasm
}  // namespace lynx
