// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/matrix_clip_tracker.h"

#include "clay/fml/logging.h"
#ifndef ENABLE_SKITY
#include "clay/gfx/skity_to_skia_utils.h"
#endif
#include "skity/geometry/rect.hpp"

namespace clay {

class Data4x4 : public MatrixClipTracker::Data {
 public:
  Data4x4(const skity::Matrix& m44, const skity::Rect& rect)
      : Data(rect), m44_(m44) {}
  explicit Data4x4(const Data* copy)
      : Data(copy->device_cull_rect()), m44_(copy->matrix_4x4()) {}

  ~Data4x4() override = default;

  bool is_4x4() const override { return true; }

  skity::Matrix matrix_4x4() const override { return m44_; }
  skity::Rect local_cull_rect() const override;

  void translate(float tx, float ty) override { m44_.PreTranslate(tx, ty); }
  void scale(float sx, float sy) override { m44_.PreScale(sx, sy); }
  void skew(float skx, float sky) override {
    m44_.PreConcat(skity::Matrix::Skew(skx, sky));
  }
  void rotate(float degrees) override {
    m44_.PreConcat(skity::Matrix::RotateDeg(degrees));
  }
  void transform(const skity::Matrix& matrix) override {
    m44_.PreConcat(matrix);
  }
  void setTransform(const skity::Matrix& matrix) override { m44_ = matrix; }
  void setIdentity() override { m44_.Reset(); }
  bool mapRect(const skity::Rect& rect, skity::Rect* mapped) const override {
    return m44_.MapRect(mapped, rect);
  }
  bool canBeInverted() const override { return m44_.Invert(nullptr); }

 protected:
  bool has_perspective() const override;

 private:
  skity::Matrix m44_;
};

static constexpr skity::Rect kMaxCullRect =
    skity::Rect::MakeLTRB(-1E9F, -1E9F, 1E9F, 1E9F);

MatrixClipTracker::MatrixClipTracker(const skity::Rect& cull_rect,
                                     const skity::Matrix& matrix) {
  // isEmpty protects us against NaN as we normalize any empty cull rects
  skity::Rect cull = cull_rect.IsEmpty() ? skity::Rect::MakeEmpty() : cull_rect;
  saved_.emplace_back(std::make_unique<Data4x4>(matrix, cull));
  current_ = saved_.back().get();
}

void MatrixClipTracker::transform2DAffine(float mxx, float mxy, float mxt,
                                          float myx, float myy, float myt) {
  transform(skity::Matrix(  //
      mxx, myx, 0, 0,       //
      mxy, myy, 0, 0,       //
      0, 0, 1, 0,           //
      mxt, myt, 0, 1));
}
void MatrixClipTracker::transformFullPerspective(
    float mxx, float mxy, float mxz, float mxt, float myx, float myy, float myz,
    float myt, float mzx, float mzy, float mzz, float mzt, float mwx, float mwy,
    float mwz, float mwt) {
  transform(skity::Matrix(  //
      mxx, myx, mzx, mwx,   //
      mxy, myy, mzy, mwy,   //
      mxz, myz, mzz, mwz,   //
      mxt, myt, mzt, mwt));
}

void MatrixClipTracker::save() {
  saved_.emplace_back(std::make_unique<Data4x4>(current_));
  current_ = saved_.back().get();
}

void MatrixClipTracker::restore() {
  saved_.pop_back();
  current_ = saved_.back().get();
}

void MatrixClipTracker::restoreToCount(int restore_count) {
  FML_DCHECK(restore_count <= getSaveCount());
  if (restore_count < 1) {
    restore_count = 1;
  }
  while (restore_count < getSaveCount()) {
    restore();
  }
}

void MatrixClipTracker::clipRRect(const skity::RRect& rrect, clay::GrClipOp op,
                                  bool is_aa) {
  switch (op) {
    case clay::GrClipOp::kIntersect:
      break;
    case clay::GrClipOp::kDifference:
      if (!rrect.IsRect()) {
        return;
      }
      break;
    default:
      break;
  }
  current_->clipBounds(rrect.GetBounds(), op, is_aa);
}

void MatrixClipTracker::clipPath(const clay::GrPath& path, clay::GrClipOp op,
                                 bool is_aa) {
  // Map "kDifference of inverse path" to "kIntersect of the original path" and
  // map "kIntersect of inverse path" to "kDifference of the original path"
#ifndef ENABLE_SKITY
  if (path.isInverseFillType()) {
    switch (op) {
      case clay::GrClipOp::kIntersect:
        op = clay::GrClipOp::kDifference;
        break;
      case clay::GrClipOp::kDifference:
        op = clay::GrClipOp::kIntersect;
        break;
      default:
        break;
    }
  }
#endif  // ENABLE_SKITY

  clay::GrRect bounds;
  switch (op) {
    case clay::GrClipOp::kIntersect:
      // bounds = path.getBounds();
      bounds = PATH_GET_BOUNDS(path);
      break;
    case clay::GrClipOp::kDifference:
      if (!PATH_IS_RECT(path, &bounds)) {
        return;
      }
      break;
    default:
      break;
  }
#ifndef ENABLE_SKITY
  current_->clipBounds(ConvertSkRectToSkityRect(bounds), op, is_aa);
#else
  current_->clipBounds(bounds, op, is_aa);
#endif  // ENABLE_SKITY
}

bool MatrixClipTracker::Data::content_culled(
    const skity::Rect& content_bounds) const {
  if (cull_rect_.IsEmpty() || content_bounds.IsEmpty()) {
    return true;
  }
  if (!canBeInverted()) {
    return true;
  }
  if (has_perspective()) {
    return false;
  }
  skity::Rect mapped;
  mapRect(content_bounds, &mapped);
  return !mapped.Intersect(cull_rect_);
}

void MatrixClipTracker::Data::clipBounds(const skity::Rect& clip,
                                         clay::GrClipOp op, bool is_aa) {
  if (cull_rect_.IsEmpty()) {
    // No point in intersecting further.
    return;
  }
  if (has_perspective()) {
    // We can conservatively ignore this clip.
    return;
  }
  switch (op) {
    case clay::GrClipOp::kIntersect: {
      if (clip.IsEmpty()) {
        cull_rect_.SetEmpty();
        break;
      }
      skity::Rect rect;
      mapRect(clip, &rect);
      if (is_aa) {
        rect.RoundOut();
      }
      if (!cull_rect_.Intersect(rect)) {
        cull_rect_.SetEmpty();
      }
      break;
    }
    case clay::GrClipOp::kDifference: {
      if (clip.IsEmpty()) {
        break;
      }
      skity::Rect rect;
      if (mapRect(clip, &rect)) {
        // This technique only works if the transform is rect -> rect
        if (is_aa) {
          skity::Rect rounded = rect;
          rounded.Round();
          if (rounded.IsEmpty()) {
            break;
          }
          rect = rounded;
        }
        if (!rect.Intersect(cull_rect_)) {
          break;
        }
        if (rect.Left() <= cull_rect_.Left() &&
            rect.Right() >= cull_rect_.Right()) {
          // bounds spans entire width of cull_rect_
          // therefore we can slice off a top or bottom
          // edge of the cull_rect_.
          float top = cull_rect_.Top();
          float btm = cull_rect_.Bottom();
          if (rect.Top() <= top) {
            top = rect.Bottom();
          }
          if (rect.Bottom() >= btm) {
            btm = rect.Top();
          }
          if (top < btm) {
            cull_rect_.SetTop(top);
            cull_rect_.SetBottom(btm);
          } else {
            cull_rect_.SetEmpty();
          }
        } else if (rect.Top() <= cull_rect_.Top() &&
                   rect.Bottom() >= cull_rect_.Bottom()) {
          // bounds spans entire height of cull_rect_
          // therefore we can slice off a left or right
          // edge of the cull_rect_.
          float lft = cull_rect_.Left();
          float rgt = cull_rect_.Right();
          if (rect.Left() <= lft) {
            lft = rect.Right();
          }
          if (rect.Right() >= rgt) {
            rgt = rect.Left();
          }
          if (lft < rgt) {
            cull_rect_.SetLeft(lft);
            cull_rect_.SetRight(rgt);
          } else {
            cull_rect_.SetEmpty();
          }
        }
      }
      break;
    }
    default:
      break;
  }
}

skity::Rect Data4x4::local_cull_rect() const {
  if (cull_rect_.IsEmpty()) {
    return cull_rect_;
  }
  skity::Matrix inverse;
  if (!m44_.Invert(&inverse)) {
    return skity::Rect::MakeEmpty();
  }
  if (has_perspective()) {
    // We could do a 4-point long-form conversion, but since this is
    // only used for culling, let's just return a non-constricting
    // cull rect.
    return kMaxCullRect;
  }
  return inverse.MapRect(cull_rect_);
}

bool Data4x4::has_perspective() const { return m44_.HasPersp(); }

}  // namespace clay
