// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_SIZE_H_
#define CLAY_GFX_GEOMETRY_SIZE_H_

namespace clay {

class Size {
 public:
  Size() : width_(0), height_(0) {}

  Size(int width, int height) : width_(width), height_(height) {}

  int width() const { return width_; }
  int height() const { return height_; }

  void SetWidth(int width) { width_ = width; }
  void SetHeight(int height) { height_ = height; }

  bool IsEmpty() const { return width_ <= 0 || height_ <= 0; }
  bool IsZero() const { return !width_ && !height_; }

  float AspectRatio() const {
    return static_cast<float>(width_) / static_cast<float>(height_);
  }

  float GetArea() const { return width() * height(); }

  void Expand(int width, int height) {
    width_ += width;
    height_ += height;
  }

 private:
  int width_;
  int height_;
};

inline Size& operator+=(Size& a, const Size& b) {
  a.SetWidth(a.width() + b.width());
  a.SetHeight(a.height() + b.height());
  return a;
}

inline Size& operator-=(Size& a, const Size& b) {
  a.SetWidth(a.width() - b.width());
  a.SetHeight(a.height() - b.height());
  return a;
}

inline Size operator+(const Size& a, const Size& b) {
  return Size(a.width() + b.width(), a.height() + b.height());
}

inline Size operator-(const Size& a, const Size& b) {
  return Size(a.width() - b.width(), a.height() - b.height());
}

inline Size operator-(const Size& size) {
  return Size(-size.width(), -size.height());
}

inline bool operator==(const Size& a, const Size& b) {
  return a.width() == b.width() && a.height() == b.height();
}

inline bool operator!=(const Size& a, const Size& b) {
  return a.width() != b.width() || a.height() != b.height();
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_SIZE_H_
