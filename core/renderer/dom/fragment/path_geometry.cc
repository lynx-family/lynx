// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/path_geometry.h"

#include <cmath>
#include <utility>

#include "core/renderer/dom/fragment/path_parser.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {
namespace {
double Length(const lepus::CArray& shape, size_t i, double extent) {
  using Unit = starlight::PlatformLengthUnit;
  const auto& value = shape.get(i);
  switch (static_cast<Unit>(shape.get(i + 1).Number())) {
    case Unit::PERCENTAGE:
      return value.Number() * extent;
    case Unit::NUMBER:
      return value.Number();
    case Unit::CALC: {
      double n = 0;
      auto terms = value.Array();
      if (terms)
        for (size_t j = 0; j + 1 < terms->size(); j += 2)
          n += Length(*terms, j, extent);
      return n;
    }
    default:
      return 0;
  }
}
bool ValidExponents(float x, float y) {
  return x > 0 && y > 0 && std::isfinite(x) && std::isfinite(y);
}
}  // namespace

bool ResolveClipOutline(const lepus::CArray& shape, float w, float h,
                        float scale, PathGeometry& out) {
  if (shape.size() == 0) return false;
  using Kind = starlight::BasicShapeType;
  auto len = [&](size_t i, float extent) {
    return static_cast<float>(Length(shape, i, extent));
  };
  switch (static_cast<Kind>(shape.get(0).Number())) {
    case Kind::kPath: {
      if (shape.size() != 2 || !shape.get(1).IsString()) return false;
      Path path;
      if (!ParseSvgPath(shape.get(1).StdString(), scale, path)) return false;
      out = std::move(path);
      return true;
    }
    case Kind::kCircle:
      if (shape.size() != 7) return false;
      out = CirclePath{len(3, w), len(5, h),
                       len(1, std::hypot(w, h) / std::sqrt(2.f))};
      return true;
    case Kind::kEllipse:
      if (shape.size() != 9) return false;
      out = EllipsePath{len(5, w), len(7, h), len(1, w), len(3, h)};
      return true;
    case Kind::kSuperEllipse: {
      if (shape.size() != 11) return false;
      float ex = shape.get(5).Number(), ey = shape.get(6).Number();
      if (!ValidExponents(ex, ey)) return false;
      out =
          SuperEllipsePath{len(7, w), len(9, h), len(1, w), len(3, h), ex, ey};
      return true;
    }
    case Kind::kInset: {
      if (shape.size() != 9 && shape.size() != 25 && shape.size() != 27)
        return false;
      RectPath inset;
      auto& rect = inset.rect;
      const float left = len(7, w), top = len(1, h);
      const float width = (w - len(3, w)) - left,
                  height = (h - len(5, h)) - top;
      rect.SetRect({{left, top}, {width, height}});
      const bool super = shape.size() == 27;
      if (super) {
        inset.exponent_x = shape.get(9).Number();
        inset.exponent_y = shape.get(10).Number();
        if (!ValidExponents(inset.exponent_x, inset.exponent_y)) return false;
      }
      if (shape.size() > 9) {
        const size_t i = super ? 11 : 9;
        rect.SetRadiusXTopLeft(len(i, width));
        rect.SetRadiusYTopLeft(len(i + 2, height));
        rect.SetRadiusXTopRight(len(i + 4, width));
        rect.SetRadiusYTopRight(len(i + 6, height));
        rect.SetRadiusXBottomRight(len(i + 8, width));
        rect.SetRadiusYBottomRight(len(i + 10, height));
        rect.SetRadiusXBottomLeft(len(i + 12, width));
        rect.SetRadiusYBottomLeft(len(i + 14, height));
      }
      out = inset;
      return true;
    }
    default:
      return false;
  }
}

const PathGeometry* ClipPathCache::Resolve(
    const fml::RefPtr<lepus::CArray>& style, float width, float height,
    float css_scale) {
  if (style_ != style || width_ != width || height_ != height ||
      css_scale_ != css_scale) {
    style_ = style;
    width_ = width;
    height_ = height;
    css_scale_ = css_scale;
    valid_ =
        style && ResolveClipOutline(*style, width, height, css_scale, path_);
  }
  return valid_ ? &path_ : nullptr;
}

}  // namespace tasm
}  // namespace lynx
