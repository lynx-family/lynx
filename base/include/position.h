// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_POSITION_H_
#define BASE_INCLUDE_POSITION_H_

#include "base/include/vector2d.h"

namespace lynx {
namespace base {

class Position {
 public:
  Position() : left_(0), top_(0), right_(0), bottom_(0) {}

  Position(int l, int t, int r, int b)
      : left_(l), top_(t), right_(r), bottom_(b) {}

  Position(const Position& pos)
      : left_(pos.left_),
        top_(pos.top_),
        right_(pos.right_),
        bottom_(pos.bottom_) {}

  ~Position() {}

  void Update(Vector2D& vec) {
    left_ += vec.x();
    top_ += vec.y();
    right_ += vec.x();
    bottom_ += vec.y();
  }

  Vector2D Offset() { return Vector2D(left_, top_); }

  bool Reset(const Position& position) {
    if (left_ == position.left_ && top_ == position.top_ &&
        right_ == position.right_ && bottom_ == position.bottom_) {
      return false;
    }

    left_ = position.left_;
    top_ = position.top_;
    right_ = position.right_;
    bottom_ = position.bottom_;
    return true;
  }

  bool Reset(int left, int top, int right, int bottom) {
    if (left_ == left && top_ == top && right_ == right && bottom_ == bottom) {
      return false;
    }

    left_ = left;
    top_ = top;
    right_ = right;
    bottom_ = bottom;
    return true;
  }

  bool Equal(int left, int top, int right, int bottom) {
    return (left_ == left && top_ == top && right_ == right &&
            bottom_ == bottom);
  }

  inline int GetWidth() const {
    int width = right_ - left_;
    return width > 0 ? width : 0;
  }

  inline int GetHeight() const {
    int height = bottom_ - top_;
    return height > 0 ? height : 0;
  }

  inline bool IsEmpty() const {
    return !(left_ || top_ || right_ || bottom_) || GetWidth() == 0 ||
           GetHeight() == 0;
  }

 public:
  int left_;
  int top_;
  int right_;
  int bottom_;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_POSITION_H_
