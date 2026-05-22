// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cassert>
#include <skity/geometry/rrect.hpp>

#include "src/geometry/math.hpp"

namespace skity {
namespace clay {

static bool are_radius_check_predicates_valid(float rad, float min, float max) {
  return (min <= max) && (rad <= max - min) && (min + rad <= max) &&
         (max - rad >= min) && rad >= 0;
}

RRect::Type RRect::GetType() const { return type_; }

void RRect::SetRect(Rect const& rect) {
  if (!this->InitializeRect(rect)) {
    return;
  }

  this->radii_[0] = {0, 0};
  this->radii_[1] = {0, 0};
  this->radii_[2] = {0, 0};
  this->radii_[3] = {0, 0};

  this->type_ = Type::kRect;

  assert(this->IsValid());
}
namespace {
float CalculateMinScale(float r1, float r2, float limit, float curr_scale) {
  if (r1 + r2 > limit) {
    return std::min(curr_scale, SkityIEEEFloatDivided(limit, (r1 + r2)));
  }
  return curr_scale;
}
}  // namespace

bool RRect::ScaleRadii() {
  float scale = 1.0f;
  scale =
      CalculateMinScale(radii_[0].x, radii_[1].x, rect_.Width(), scale);  // Top
  scale = CalculateMinScale(radii_[1].y, radii_[2].y, rect_.Height(),
                            scale);  // Right
  scale = CalculateMinScale(radii_[2].x, radii_[3].x, rect_.Width(),
                            scale);  // Bottom
  scale = CalculateMinScale(radii_[3].y, radii_[0].y, rect_.Height(),
                            scale);  // Left
  if (scale < 1.0f) {
    radii_[0] *= scale;
    radii_[1] *= scale;
    radii_[2] *= scale;
    radii_[3] *= scale;
  }

  return scale < 1.0f;
}

void RRect::SetRectRadii(Rect const& rect, const Vec2 radii[4]) {
  if (!this->InitializeRect(rect)) {
    return;
  }
  this->radii_[0] = radii[0];
  this->radii_[1] = radii[1];
  this->radii_[2] = radii[2];
  this->radii_[3] = radii[3];

  ScaleRadii();

  this->type_ = Type::kComplex;
}

void RRect::SetRectXY(Rect const& rect, float xRad, float yRad) {
  if (!this->InitializeRect(rect)) {
    return;
  }

  if (!FloatIsFinite(xRad) && !FloatIsFinite(yRad)) {
    xRad = yRad = 0;
  }

  if (rect_.Width() < xRad + xRad || rect_.Height() < yRad + yRad) {
    float scale = std::min(SkityIEEEFloatDivided(rect_.Width(), xRad + xRad),
                           SkityIEEEFloatDivided(rect_.Height(), yRad + yRad));
    assert(scale < Float1);
    xRad *= scale;
    yRad *= scale;
  }

  if (xRad <= 0 || yRad <= 0) {
    this->SetRect(rect);
    return;
  }

  for (int32_t i = 0; i < 4; i++) {
    radii_[i].x = xRad;
    radii_[i].y = yRad;
  }

  type_ = Type::kSimple;

  if (xRad >= FloatHalf * rect_.Width() && yRad >= FloatHalf * rect_.Height()) {
    type_ = Type::kOval;
  }

  assert(this->IsValid());
}

void RRect::SetOval(Rect const& oval) {
  if (!this->InitializeRect(oval)) {
    return;
  }

  float x_rad = Rect::HalfWidth(rect_);
  float y_rad = Rect::HalfHeight(rect_);

  if (x_rad == 0.f || y_rad == 0.f) {
    type_ = kRect;
  } else {
    for (int32_t i = 0; i < 4; i++) {
      radii_[i].x = x_rad;
      radii_[i].y = y_rad;
    }
    type_ = kOval;
  }

  assert(this->IsValid());
}

bool RRect::IsValid() const {
  if (!AreRectAndRadiiValid(rect_, radii_)) {
    return false;
  }

  bool all_radii_zero = (0 == radii_[0].x && 0 == radii_[0].y);
  bool all_corners_square = (0 == radii_[0].x && 0 == radii_[0].y);
  bool all_radii_same = true;

  for (int32_t i = 1; i < 4; i++) {
    if (0 != radii_[i].x || 0 != radii_[i].y) {
      all_radii_zero = false;
    }

    if (radii_[i].x != radii_[i - 1].x || radii_[i].y != radii_[i - 1].y) {
      all_radii_same = false;
    }

    if (0 != radii_[i].x && 0 != radii_[i].y) {
      all_corners_square = false;
    }
  }

  // TODO(tangruiwen) support nine patch

  if (static_cast<int32_t>(type_) < 0 ||
      static_cast<int32_t>(type_) > kLastType) {
    return false;
  }

  switch (type_) {
    case Type::kEmpty:
      if (!rect_.IsEmpty() || !all_radii_zero || !all_radii_same ||
          !all_corners_square) {
        return false;
      }
      break;
    case Type::kRect:
      if (rect_.IsEmpty() || !all_radii_zero || !all_radii_same ||
          !all_corners_square) {
        return false;
      }
      break;
    case Type::kOval:
      if (rect_.IsEmpty() || all_radii_zero || !all_radii_same ||
          all_corners_square) {
        return false;
      }
      for (int32_t i = 0; i < 4; i++) {
        if (!FloatNearlyZero(radii_[i].x, Rect::HalfWidth(rect_)) ||
            !FloatNearlyZero(radii_[i].y, Rect::HalfHeight(rect_))) {
          return false;
        }
      }
      break;
    case Type::kSimple:
      if (rect_.IsEmpty() || all_radii_zero || !all_radii_same ||
          all_corners_square) {
        return false;
      }
      break;
    case Type::kNinePatch:
      return false;
    case Type::kComplex:
      if (rect_.IsEmpty() || all_radii_zero || all_radii_same ||
          all_corners_square) {
        return false;
      }
      break;
  }

  return true;
}

bool RRect::AreRectAndRadiiValid(Rect const& rect,
                                 std::array<Vec2, 4> const& radii) {
  if (!rect.IsFinite() || !rect.IsSorted()) {
    return false;
  }

  for (int32_t i = 0; i < 4; i++) {
    if (!are_radius_check_predicates_valid(radii[i].x, rect.Left(),
                                           rect.Right()) ||
        !are_radius_check_predicates_valid(radii[i].y, rect.Top(),
                                           rect.Bottom())) {
      return false;
    }
  }

  return true;
}

RRect RRect::MakeOffset(float dx, float dy) const {
  RRect rr;
  rr.SetRectRadii(rect_.MakeOffset(dx, dy), radii_.data());
  return rr;
}

RRect RRect::MakeEmpty() { return RRect(); }

RRect RRect::MakeRect(Rect const& rect) {
  RRect rr;
  rr.SetRect(rect);
  return rr;
}

RRect RRect::MakeRectXY(Rect const& rect, float xRad, float yRad) {
  RRect rr;
  rr.SetRectXY(rect, xRad, yRad);
  return rr;
}

RRect RRect::MakeOval(Rect const& oval) {
  RRect rr;
  rr.SetOval(oval);
  return rr;
}

bool RRect::InitializeRect(Rect const& rect) {
  // Check this before sorting because sorting can hide nans.
  if (!rect.IsFinite()) {
    *this = RRect();
    return false;
  }

  rect_ = rect.MakeSorted();
  if (rect_.IsEmpty()) {
    type_ = Type::kEmpty;
    return false;
  }

  return true;
}

void RRect::Inset(float dx, float dy, RRect* dst) const {
  Rect r = rect_;
  r.Inset(dx, dy);

  bool degenerate = false;
  if (r.Right() <= r.Left()) {
    degenerate = true;
    r.left_ = r.right_ = (r.Left() + r.Right()) * 0.5f;
  }
  if (r.Bottom() <= r.Top()) {
    degenerate = true;
    r.top_ = r.bottom_ = (r.Top() + r.Bottom()) * 0.5f;
  }
  if (degenerate) {
    dst->rect_ = r;
    dst->radii_ = {};
    dst->type_ = Type::kEmpty;
    return;
  }
  if (!r.IsFinite()) {
    *dst = RRect();
    return;
  }

  std::array<Vec2, 4> radii = radii_;
  for (int i = 0; i < 4; ++i) {
    if (radii[i].x) {
      radii[i].x -= dx;
    }
    if (radii[i].y) {
      radii[i].y -= dy;
    }
  }
  dst->rect_ = r;
  dst->radii_ = radii;
  dst->type_ = Type::kComplex;
}

bool RRect::Contains(const Rect& rect) const {
  if (!rect_.Contains(rect)) {
    return false;
  }

  if (IsRect()) {
    return true;
  }

  return CheckCornerContainment(rect.Left(), rect.Top()) &&
         CheckCornerContainment(rect.Right(), rect.Top()) &&
         CheckCornerContainment(rect.Right(), rect.Bottom()) &&
         CheckCornerContainment(rect.Left(), rect.Bottom());
}

bool RRect::CheckCornerContainment(float x, float y) const {
  Vec2 canonical_pt;
  int index;

  if (kOval == GetType()) {
    canonical_pt = {x - rect_.CenterX(), y - rect_.CenterY()};
    index = kUpperLeft;
  } else {
    if (x < rect_.Left() + radii_[kUpperLeft].x &&
        y < rect_.Top() + radii_[kUpperLeft].y) {
      // UL corner
      index = kUpperLeft;
      canonical_pt = {x - (rect_.Left() + radii_[kUpperLeft].x),
                      y - (rect_.Top() + radii_[kUpperLeft].y)};
    } else if (x < rect_.Left() + radii_[kLowerLeft].x &&
               y > rect_.Bottom() - radii_[kLowerLeft].y) {
      // LL corner
      index = kLowerLeft;
      canonical_pt = {x - (rect_.Left() + radii_[kLowerLeft].x),
                      y - (rect_.Bottom() - radii_[kLowerLeft].y)};
    } else if (x > rect_.Right() - radii_[kUpperRight].x &&
               y < rect_.Top() + radii_[kUpperRight].y) {
      // UR corner
      index = kUpperRight;
      canonical_pt = {x - (rect_.Right() - radii_[kUpperRight].x),
                      y - (rect_.Top() + radii_[kUpperRight].y)};
    } else if (x > rect_.Right() - radii_[kLowerRight].x &&
               y > rect_.Bottom() - radii_[kLowerRight].y) {
      // LR corner
      index = kLowerRight;
      canonical_pt = {x - (rect_.Right() - radii_[kLowerRight].x),
                      y - (rect_.Bottom() - radii_[kLowerRight].y)};
    } else {
      // not in any of the corners
      return true;
    }
  }

  // A point is in an ellipse (in standard position) if:
  //      x^2     y^2
  //     ----- + ----- <= 1
  //      a^2     b^2
  // or :
  //     b^2*x^2 + a^2*y^2 <= (ab)^2
  float dist = std::sqrtf(canonical_pt.x) * std::sqrtf(radii_[index].y) +
               std::sqrtf(canonical_pt.y) * std::sqrtf(radii_[index].x);
  return dist <= std::sqrtf(radii_[index].x * radii_[index].y);
}

}  // namespace clay
}  // namespace skity
