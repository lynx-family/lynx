// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef INCLUDE_SKITY_GEOMETRY_VECTOR_HPP
#define INCLUDE_SKITY_GEOMETRY_VECTOR_HPP

#include <algorithm>
#include <skity/geometry/scalar.hpp>
#include <skity/macros.hpp>

namespace skity {
namespace clay {
struct Vec2;
struct Vec3;
struct Vec4;

struct SKITY_API Vec2 {
  union {
    struct {
      float x;
      float y;
    };
    float e[2];
  };

  constexpr Vec2() : x(0.f), y(0.f) {}

  template <typename T1, typename T2,
            typename = std::enable_if_t<std::is_arithmetic_v<T1> &&
                                        std::is_arithmetic_v<T2>>>
  constexpr Vec2(T1 x, T2 y)
      : x(static_cast<float>(x)), y(static_cast<float>(y)) {}

  constexpr explicit Vec2(float v) : x(v), y(v) {}

  constexpr explicit Vec2(const Vec4& v);

  constexpr bool operator==(const Vec2 v) const { return x == v.x && y == v.y; }
  constexpr bool operator!=(const Vec2 v) const { return !(*this == v); }

  constexpr static float Dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
  constexpr static float Cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }

  constexpr static Vec2 Min(Vec2 a, Vec2 b) {
    return Vec2{std::min(a.x, b.x), std::min(a.y, b.y)};
  }

  constexpr static Vec2 Max(Vec2 a, Vec2 b) {
    return Vec2{std::max(a.x, b.x), std::max(a.y, b.y)};
  }

  static Vec2 Normalize(Vec2 v) {
    auto length_squared = v.LengthSquared();
    if (FloatNearlyZero(length_squared)) {
      return Vec2{};
    }
    return v * (1.0f / std::sqrt(length_squared));
  }

  static Vec2 Sqrt(Vec2 v) { return {std::sqrt(v.x), std::sqrt(v.y)}; }

  static Vec2 Round(Vec2 v) { return {std::round(v.x), std::round(v.y)}; }

  static Vec2 Abs(Vec2 v) { return {std::abs(v.x), std::abs(v.y)}; }

  constexpr Vec2 operator-() const { return {-x, -y}; }
  constexpr Vec2 operator+(Vec2 v) const { return {x + v.x, y + v.y}; }
  constexpr Vec2 operator-(Vec2 v) const { return {x - v.x, y - v.y}; }
  constexpr Vec2 operator*(Vec2 v) const { return {x * v.x, y * v.y}; }
  constexpr friend Vec2 operator*(Vec2 v, float s) {
    return {v.x * s, v.y * s};
  }
  constexpr friend Vec2 operator*(float s, Vec2 v) {
    return {v.x * s, v.y * s};
  }
  constexpr friend Vec2 operator/(Vec2 v, float s) {
    return {v.x / s, v.y / s};
  }
  constexpr friend Vec2 operator/(float s, Vec2 v) {
    return {s / v.x, s / v.y};
  }
  constexpr friend Vec2 operator/(Vec2 a, Vec2 b) {
    return {a.x / b.x, a.y / b.y};
  }

  constexpr void operator+=(Vec2 v) { *this = *this + v; }
  constexpr void operator-=(Vec2 v) { *this = *this - v; }
  constexpr void operator*=(Vec2 v) { *this = *this * v; }
  constexpr void operator*=(float s) { *this = *this * s; }
  constexpr void operator/=(float s) { *this = *this / s; }

  constexpr float LengthSquared() const { return Dot(*this, *this); }
  float Length() const { return std::sqrt(this->LengthSquared()); }

  constexpr float Dot(Vec2 v) const { return Dot(*this, v); }
  constexpr float Cross(Vec2 v) const { return Cross(*this, v); }
  Vec2 Normalize() const { return Normalize(*this); }

  constexpr float operator[](int i) const { return e[i]; }
  constexpr float& operator[](int i) { return e[i]; }
};

struct SKITY_API Vec3 {
  union {
    struct {
      float x, y, z;
    };
    float e[3];
  };

  constexpr Vec3() : x(0.f), y(0.f), z(0.f) {}

  template <typename T1, typename T2, typename T3,
            typename = std::enable_if_t<std::is_arithmetic_v<T1> &&
                                        std::is_arithmetic_v<T2> &&
                                        std::is_arithmetic_v<T3>>>
  constexpr Vec3(T1 x, T2 y, T3 z)
      : x(static_cast<float>(x)),
        y(static_cast<float>(y)),
        z(static_cast<float>(z)) {}

  constexpr explicit Vec3(float v) : x(v), y(v), z(v) {}

  constexpr bool operator==(const Vec3& v) const {
    return x == v.x && y == v.y && z == v.z;
  }
  constexpr bool operator!=(const Vec3& v) const { return !(*this == v); }

  constexpr static float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }
  constexpr static Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
  }
  static Vec3 Normalize(const Vec3& v) {
    auto length_squared = v.LengthSquared();
    if (FloatNearlyZero(length_squared)) {
      return Vec3{};
    }
    return v * (1.0f / std::sqrt(length_squared));
  }

  constexpr Vec3 operator-() const { return {-x, -y, -z}; }
  constexpr Vec3 operator+(const Vec3& v) const {
    return {x + v.x, y + v.y, z + v.z};
  }
  constexpr Vec3 operator-(const Vec3& v) const {
    return {x - v.x, y - v.y, z - v.z};
  }

  constexpr Vec3 operator*(const Vec3& v) const {
    return {x * v.x, y * v.y, z * v.z};
  }
  constexpr friend Vec3 operator*(const Vec3& v, float s) {
    return {v.x * s, v.y * s, v.z * s};
  }
  constexpr friend Vec3 operator*(float s, const Vec3& v) { return v * s; }

  constexpr Vec3 operator/(const Vec3& v) const {
    return {x / v.x, y / v.y, z / v.z};
  }
  constexpr friend Vec3 operator/(const Vec3& v, float s) {
    return {v.x / s, v.y / s, v.z / s};
  }
  constexpr friend Vec3 operator/(float s, const Vec3& v) {
    return {s / v.x, s / v.y, s / v.z};
  }

  constexpr void operator+=(Vec3 v) { *this = *this + v; }
  constexpr void operator-=(Vec3 v) { *this = *this - v; }
  constexpr void operator*=(Vec3 v) { *this = *this * v; }
  constexpr void operator*=(float s) { *this = *this * s; }
  constexpr void operator/=(float s) { *this = *this / s; }

  constexpr float LengthSquared() const { return Dot(*this, *this); }
  float Length() const { return std::sqrt(Dot(*this, *this)); }

  constexpr float Dot(const Vec3& v) const { return Dot(*this, v); }
  constexpr Vec3 Cross(const Vec3& v) const { return Cross(*this, v); }
  Vec3 Normalize() const { return Normalize(*this); }

  constexpr static Vec3 Min(Vec3 a, Vec3 b) {
    return Vec3{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
  }
  constexpr static Vec3 Max(Vec3 a, Vec3 b) {
    return Vec3{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
  }

  constexpr float operator[](int i) const { return e[i]; }
  constexpr float& operator[](int i) { return e[i]; }
};

struct SKITY_API Vec4 {
  union {
    struct {
      float x, y, z, w;
    };
    struct {
      float r, g, b, a;
    };
    float e[4];
  };

  constexpr Vec4() : x(0), y(0), z(0), w(0) {}

  template <typename T1, typename T2, typename T3, typename T4,
            typename = std::enable_if_t<
                std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2> &&
                std::is_arithmetic_v<T3> && std::is_arithmetic_v<T4>>>
  constexpr Vec4(T1 x, T2 y, T3 z, T4 w)
      : x(static_cast<float>(x)),
        y(static_cast<float>(y)),
        z(static_cast<float>(z)),
        w(static_cast<float>(w)) {}

  constexpr explicit Vec4(float v) : x(v), y(v), z(v), w(v) {}

  constexpr Vec4(const Vec2& xy, float z, float w)
      : x(xy.x), y(xy.y), z(z), w(w) {}

  constexpr Vec4(const Vec2& xy, const Vec2& zw)
      : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

  constexpr Vec4(const Vec3& xyz, float w)
      : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}

  constexpr bool operator==(const Vec4& v) const {
    return x == v.x && y == v.y && z == v.z && w == v.w;
  }
  constexpr bool operator!=(const Vec4& v) const { return !(*this == v); }

  constexpr static float Dot(const Vec4& a, const Vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }
  static Vec4 Normalize(const Vec4& v) {
    auto length_squared = v.LengthSquared();
    if (FloatNearlyZero(length_squared)) {
      return Vec4{};
    }
    return v * (1.0f / std::sqrt(length_squared));
  }

  constexpr Vec4 operator-() const { return {-x, -y, -z, -w}; }
  constexpr Vec4 operator+(const Vec4& v) const {
    return {x + v.x, y + v.y, z + v.z, w + v.w};
  }
  constexpr Vec4 operator-(const Vec4& v) const {
    return {x - v.x, y - v.y, z - v.z, w - v.w};
  }

  constexpr Vec4 operator*(const Vec4& v) const {
    return {x * v.x, y * v.y, z * v.z, w * v.w};
  }
  constexpr friend Vec4 operator*(const Vec4& v, float s) {
    return {v.x * s, v.y * s, v.z * s, v.w * s};
  }
  constexpr friend Vec4 operator*(float s, const Vec4& v) { return v * s; }

  constexpr friend Vec4 operator/(const Vec4& v, float s) {
    return {v.x / s, v.y / s, v.z / s, v.w / s};
  }
  constexpr friend Vec4 operator/(float s, const Vec4& v) {
    return {s / v.x, s / v.y, s / v.z, s / v.w};
  }

  constexpr friend Vec4 operator/(const Vec4& a, const Vec4& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
  }

  constexpr static Vec4 Min(Vec4 a, Vec4 b) {
    return Vec4{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z),
                std::min(a.w, b.w)};
  }
  constexpr static Vec4 Max(Vec4 a, Vec4 b) {
    return Vec4{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z),
                std::max(a.w, b.w)};
  }

  constexpr float LengthSquared() const { return Dot(*this, *this); }
  float Length() const { return std::sqrt(Dot(*this, *this)); }

  constexpr float Dot(const Vec4& v) const { return Dot(*this, v); }
  Vec4 Normalize() const { return Normalize(*this); }

  constexpr void operator+=(Vec4 v) { *this = *this + v; }
  constexpr void operator-=(Vec4 v) { *this = *this - v; }
  constexpr void operator*=(Vec4 v) { *this = *this * v; }
  constexpr void operator*=(float s) { *this = *this * s; }
  constexpr void operator/=(float s) { *this = *this / s; }

  constexpr float operator[](int i) const { return e[i]; }
  constexpr float& operator[](int i) { return e[i]; }
};

using Vector = Vec4;

constexpr Vec2::Vec2(const Vec4& v) : x(v.x), y(v.y) {}
}  // namespace clay

using Vec2 = clay::Vec2;
using Vec3 = clay::Vec3;
using Vec4 = clay::Vec4;
using Vector = clay::Vector;

}  // namespace skity
#endif  // INCLUDE_SKITY_GEOMETRY_VECTOR_HPP
