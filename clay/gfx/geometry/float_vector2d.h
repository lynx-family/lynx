// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_FLOAT_VECTOR2D_H_
#define CLAY_GFX_GEOMETRY_FLOAT_VECTOR2D_H_

#include <string>

namespace clay {

class FloatVector2d {
 public:
  constexpr FloatVector2d() : x_(0), y_(0) {}
  constexpr FloatVector2d(float x, float y) : x_(x), y_(y) {}

  constexpr float x() const { return x_; }
  void SetX(float x) { x_ = x; }

  constexpr float y() const { return y_; }
  void SetY(float y) { y_ = y; }

  // True if both components of the vector are 0.
  bool IsZero() const;

  // Add the components of the |other| vector to the current vector.
  void Add(const FloatVector2d& other);
  // Subtract the components of the |other| vector from the current vector.
  void Subtract(const FloatVector2d& other);

  void operator+=(const FloatVector2d& other) { Add(other); }
  void operator-=(const FloatVector2d& other) { Subtract(other); }

  void SetToMin(const FloatVector2d& other) {
    x_ = x_ <= other.x_ ? x_ : other.x_;
    y_ = y_ <= other.y_ ? y_ : other.y_;
  }

  void SetToMax(const FloatVector2d& other) {
    x_ = x_ >= other.x_ ? x_ : other.x_;
    y_ = y_ >= other.y_ ? y_ : other.y_;
  }

  // Gives the square of the diagonal length of the vector.
  double LengthSquared() const;
  // Gives the diagonal length of the vector.
  float Length() const;

  // Scale the x and y components of the vector by |scale|.
  void Scale(float scale) { Scale(scale, scale); }
  // Scale the x and y components of the vector by |x_scale| and |y_scale|
  // respectively.
  void Scale(float x_scale, float y_scale);

  std::string ToString() const;

 private:
  float x_;
  float y_;
};

inline constexpr bool operator==(const FloatVector2d& lhs,
                                 const FloatVector2d& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}

inline constexpr bool operator!=(const FloatVector2d& lhs,
                                 const FloatVector2d& rhs) {
  return !(lhs == rhs);
}

inline constexpr FloatVector2d operator-(const FloatVector2d& v) {
  return FloatVector2d(-v.x(), -v.y());
}

inline FloatVector2d operator+(const FloatVector2d& lhs,
                               const FloatVector2d& rhs) {
  FloatVector2d result = lhs;
  result.Add(rhs);
  return result;
}

inline FloatVector2d operator-(const FloatVector2d& lhs,
                               const FloatVector2d& rhs) {
  FloatVector2d result = lhs;
  result.Add(-rhs);
  return result;
}

// Return the cross product of two vectors.
double CrossProduct(const FloatVector2d& lhs, const FloatVector2d& rhs);

// Return the dot product of two vectors.
double DotProduct(const FloatVector2d& lhs, const FloatVector2d& rhs);

// Return a vector that is |v| scaled by the given scale factors along each
// axis.
FloatVector2d ScaleVector2d(const FloatVector2d& v, float x_scale,
                            float y_scale);

// Return a vector that is |v| scaled by the given scale factor.
inline FloatVector2d ScaleVector2d(const FloatVector2d& v, float scale) {
  return ScaleVector2d(v, scale, scale);
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_VECTOR2D_H_
