// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_GEOMETRY_SIZE_H_
#define BASE_INCLUDE_GEOMETRY_SIZE_H_

namespace lynx {
namespace base {
namespace geometry {

template <typename T>
class Size {
 public:
  Size() : width_(0), height_(0) {}
  Size(T width, T height) : width_(width), height_(height) {}

  T Width() const { return width_; }
  T Height() const { return height_; }

  void SetWidth(T width) { width_ = width; }
  void SetHeight(T height) { height_ = height; }

  bool IsEmpty() const { return width_ == 0 && height_ == 0; }

  void Expand(T width, T height) {
    width_ += width;
    height_ += height;
  }

  Size<T> ExpandedTo(const Size<T>& other) const {
    return Size<T>(width_ > other.width_ ? width_ : other.width_,
                   height_ > other.height_ ? height_ : other.height_);
  }

 private:
  T width_;
  T height_;
};

template <typename T>
inline Size<T>& operator+=(Size<T>& a, const Size<T>& b) {
  a.SetWidth(a.Width() + b.Width());
  a.SetHeight(a.Height() + b.Height());
  return a;
}

template <typename T>
inline Size<T>& operator-=(Size<T>& a, const Size<T>& b) {
  a.SetWidth(a.Width() - b.Width());
  a.SetHeight(a.Height() - b.Height());
  return a;
}

template <typename T>
inline Size<T> operator+(const Size<T>& a, const Size<T>& b) {
  return Size<T>(a.Width() + b.Width(), a.Height() + b.Height());
}

template <typename T>
inline Size<T> operator-(const Size<T>& a, const Size<T>& b) {
  return Size<T>(a.Width() - b.Width(), a.Height() - b.Height());
}

template <typename T>
inline Size<T> operator-(const Size<T>& size) {
  return Size<T>(-size.Width(), -size.Height());
}

template <typename T>
inline bool operator==(const Size<T>& a, const Size<T>& b) {
  return a.Width() == b.Width() && a.Height() == b.Height();
}

template <typename T>
inline bool operator!=(const Size<T>& a, const Size<T>& b) {
  return a.Width() != b.Width() || a.Height() != b.Height();
}

using IntSize = Size<int>;
using FloatSize = Size<float>;

}  // namespace geometry
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_GEOMETRY_SIZE_H_
