// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef INCLUDE_SKITY_GEOMETRY_RRECT_HPP
#define INCLUDE_SKITY_GEOMETRY_RRECT_HPP

#include <array>
#include <skity/geometry/point.hpp>
#include <skity/geometry/rect.hpp>
#include <skity/macros.hpp>

namespace skity {
namespace clay {

class SKITY_API RRect {
 public:
  RRect() = default;
  RRect(RRect const& rrect) = default;
  RRect& operator=(RRect const& rrect) = default;

  bool operator==(const RRect& other) const {
    return type_ == other.type_ && rect_ == other.rect_ &&
           radii_ == other.radii_;
  }

  bool operator!=(const RRect& other) const { return !(*this == other); }

  /**
   * @enum RRect::Type
   *  Type describes possible specializations of SkRRect. Each Type is
   * 	exclusive; a RRect may only have one type.
   */
  enum Type {
    // zero width or height
    kEmpty,
    // non-zero width and height, and zeroed radii
    kRect,
    // non-zero width and height filled with radii
    kOval,
    // non-zero width and height with equal radii
    kSimple,
    // non-zero width and height with axis-aligned radii
    kNinePatch,
    // non-zero width and height with arbitrary radii
    kComplex,
    kLastType = kComplex,
  };

  /**
   * @enum RRect::Corner
   *	The radii are stored: top-left, top-right, bottom-right, bottom-left.
   */
  enum Corner {
    // index of top-left corner radii
    kUpperLeft,
    // index of top-right corner radii
    kUpperRight,
    // index of bottom-right corner radii
    kLowerRight,
    // index of bottom-left corner radii
    kLowerLeft,
  };

  Type GetType() const;

  bool IsEmpty() const { return Type::kEmpty == this->GetType(); }
  bool IsRect() const { return Type::kRect == this->GetType(); }
  bool IsOval() const { return Type::kOval == this->GetType(); }
  bool IsSimple() const { return Type::kSimple == this->GetType(); }
  bool IsNinePatch() const { return Type::kNinePatch == this->GetType(); }
  bool IsComplex() const { return Type::kComplex == this->GetType(); }

  float Width() const { return rect_.Width(); }
  float Height() const { return rect_.Height(); }
  Vec2 GetSimpleRadii() const { return radii_[0]; }

  void SetEmpty() { *this = RRect(); }

  /**
   * @brief Set the Rect object
   *
   * @param rect bounds to set
   */
  void SetRect(Rect const& rect);

  /**
   * Sets bounds to oval, x-axis radii to half oval.width(), and all y-axis
   * radii to half oval.height(). If oval bounds is empty, sets to kEmpty.
   * Otherwise, sets to kOval.
   *
   * @param oval  bounds of oval
   */
  void SetOval(Rect const& oval);

  /**
   * @brief Sets to rounded rectangle with the same radii for all four corners.
   *
   * @param rect  bounds of rounded rectangle
   * @param xRad  x-axis radius of corners
   * @param yRad  y-axis radius of corners
   */
  void SetRectXY(Rect const& rect, float xRad, float yRad);

  Rect const& GetRect() const { return rect_; }

  /**
   * @brief Sets bounds to rect. Sets radii array for individual control of all
   * for corners.
   *
   * @param rect   bounds of rounded rectangle
   * @param radii  corner x-axis and y-axis radii
   */
  void SetRectRadii(Rect const& rect, const Vec2 radii[4]);

  /**
   * Check if bounds and radii match type
   *
   * @return true
   * @return false
   */
  bool IsValid() const;

  Vec2 Radii(Corner corner) const { return radii_[corner]; }

  const Vec2* Radii() const { return radii_.data(); }

  /**
   * Translates RRect by (dx, dy)
   *
   * @param dx  offset added to rect().left() and rect().right()
   * @param dy  offset added to rect().top() and rect().bottom()
   */
  void Offset(float dx, float dy) { rect_.Offset(dx, dy); }

  void Inset(float dx, float dy, RRect* dst) const;

  void Outset(float dx, float dy, RRect* dst) const {
    this->Inset(-dx, -dy, dst);
  }

  /**
   * Returns bounds. Bounds may have zero width or zero height
   *
   * @return bounding box
   */
  Rect const& GetBounds() const { return rect_; }

  bool Contains(const Rect& rect) const;

  RRect MakeOffset(float dx, float dy) const;

  static RRect MakeEmpty();
  static RRect MakeRect(Rect const& r);
  static RRect MakeRectXY(Rect const& rect, float xRad, float yRad);
  static RRect MakeOval(Rect const& oval);

 private:
  RRect(Rect const& rect, std::array<Vec2, 4> const& radii, Type type)
      : rect_(rect), radii_(radii), type_(type) {}

  static bool AreRectAndRadiiValid(Rect const& rect,
                                   std::array<Vec2, 4> const& radii);

  bool InitializeRect(Rect const& rect);
  void ComputeType();
  bool CheckCornerContainment(float x, float y) const;
  // return true if the radii had to be scaled to fit rect
  bool ScaleRadii();

 private:
  Rect rect_ = Rect::MakeEmpty();
  std::array<Vec2, 4> radii_ = {};
  Type type_ = Type::kEmpty;
};

}  // namespace clay
using RRect = clay::RRect;
}  // namespace skity

#endif  // INCLUDE_SKITY_GEOMETRY_RRECT_HPP
