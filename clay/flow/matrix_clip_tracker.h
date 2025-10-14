// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_MATRIX_CLIP_TRACKER_H_
#define CLAY_FLOW_MATRIX_CLIP_TRACKER_H_

#include <algorithm>
#include <memory>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/rendering_backend.h"
#include "skity/geometry/matrix.hpp"
#include "skity/geometry/rect.hpp"
#include "skity/geometry/rrect.hpp"

namespace clay {

class MatrixClipTracker {
 public:
  MatrixClipTracker(const skity::Rect& cull_rect, const skity::Matrix& matrix);

  bool using_4x4_matrix() { return current_->is_4x4(); }

  skity::Matrix matrix_4x4() const { return current_->matrix_4x4(); }
  skity::Rect local_cull_rect() const { return current_->local_cull_rect(); }
  skity::Rect device_cull_rect() const { return current_->device_cull_rect(); }
  bool content_culled(const skity::Rect& content_bounds) const {
    return current_->content_culled(content_bounds);
  }

  void save();
  void restore();
  int getSaveCount() { return saved_.size(); }
  void restoreToCount(int restore_count);

  void translate(float tx, float ty) { current_->translate(tx, ty); }
  void scale(float sx, float sy) { current_->scale(sx, sy); }
  void skew(float skx, float sky) { current_->skew(skx, sky); }
  void rotate(float degrees) { current_->rotate(degrees); }
  void transform(const skity::Matrix& matrix) { current_->transform(matrix); }
  // clang-format off
  void transform2DAffine(
      float mxx, float mxy, float mxt,
      float myx, float myy, float myt);
  void transformFullPerspective(
      float mxx, float mxy, float mxz, float mxt,
      float myx, float myy, float myz, float myt,
      float mzx, float mzy, float mzz, float mzt,
      float mwx, float mwy, float mwz, float mwt);
  // clang-format on
  void setTransform(const skity::Matrix& matrix) {
    current_->setTransform(matrix);
  }
  void setIdentity() { current_->setIdentity(); }
  bool mapRect(skity::Rect* rect) const {
    return current_->mapRect(*rect, rect);
  }

  void clipRect(const skity::Rect& rect, clay::GrClipOp op, bool is_aa) {
    current_->clipBounds(rect, op, is_aa);
  }
  void clipRRect(const skity::RRect& rrect, clay::GrClipOp op, bool is_aa);
  void clipPath(const clay::GrPath& path, clay::GrClipOp op, bool is_aa);

 private:
  class Data {
   public:
    virtual ~Data() = default;

    virtual bool is_4x4() const = 0;

    virtual skity::Matrix matrix_4x4() const = 0;

    virtual skity::Rect device_cull_rect() const { return cull_rect_; }
    virtual skity::Rect local_cull_rect() const = 0;
    virtual bool content_culled(const skity::Rect& content_bounds) const;

    virtual void translate(float tx, float ty) = 0;
    virtual void scale(float sx, float sy) = 0;
    virtual void skew(float skx, float sky) = 0;
    virtual void rotate(float degrees) = 0;
    virtual void transform(const skity::Matrix& matrix) = 0;
    virtual void setTransform(const skity::Matrix& matrix) = 0;
    virtual void setIdentity() = 0;
    virtual bool mapRect(const skity::Rect& rect,
                         skity::Rect* mapped) const = 0;
    virtual bool canBeInverted() const = 0;

    virtual void clipBounds(const skity::Rect& clip, clay::GrClipOp op,
                            bool is_aa);

   protected:
    Data(const skity::Rect& rect) : cull_rect_(rect) {}

    virtual bool has_perspective() const = 0;

    skity::Rect cull_rect_;
  };
  friend class Data3x3;
  friend class Data4x4;

  Data* current_;
  std::vector<std::unique_ptr<Data>> saved_;
};

}  // namespace clay

#endif  // CLAY_FLOW_MATRIX_CLIP_TRACKER_H_
