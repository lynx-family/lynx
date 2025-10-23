// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_H_

#include <string>

#include "clay/gfx/geometry/float_vector2d.h"
#include "skity/geometry/matrix.hpp"

namespace clay {

class Point;
class FloatPoint;
class FloatPoint3d;
class Quaternion;
class FloatVector3d;

// 4x4 transformation matrix. Transform is cheap and explicitly allows
// copy/assign.
class Transform {
 public:
  enum SkipInitialization { kSkipInitialization };

  Transform() = default;

  // Initialize with the concatenation of lhs * rhs.
  Transform(const Transform& lhs, const Transform& rhs)
      : matrix_(lhs.matrix_ * rhs.matrix_) {}
  explicit Transform(const skity::Matrix& matrix) : matrix_(matrix) {}
  // Constructs a transform from explicit 16 matrix elements. Elements
  // should be given in row-major order.
  Transform(float col1row1, float col2row1, float col3row1, float col4row1,
            float col1row2, float col2row2, float col3row2, float col4row2,
            float col1row3, float col2row3, float col3row3, float col4row3,
            float col1row4, float col2row4, float col3row4, float col4row4);
  // Constructs a transform from explicit 2d elements. All other matrix
  // elements remain the same as the corresponding elements of an identity
  // matrix.
  Transform(float col1row1, float col2row1, float col1row2, float col2row2,
            float x_translation, float y_translation);

  // Constructs a transform corresponding to the given quaternion.
  explicit Transform(const Quaternion& q);

  bool operator==(const Transform& rhs) const { return matrix_ == rhs.matrix_; }
  bool operator!=(const Transform& rhs) const { return matrix_ != rhs.matrix_; }

  // Resets this transform to the identity transform.
  void MakeIdentity() { matrix_.Reset(); }

  // Applies the current transformation on a 2d rotation and assigns the result
  // to |this|.
  void Rotate(double degrees) { RotateAboutZAxis(degrees); }

  // Applies the current transformation on an axis-angle rotation and assigns
  // the result to |this|.
  void RotateAboutXAxis(double degrees);
  void RotateAboutYAxis(double degrees);
  void RotateAboutZAxis(double degrees);
  void RotateAbout(const FloatVector3d& axis, double degrees);

  // Applies the current transformation on a scaling and assigns the result
  // to |this|.
  void Scale(float x, float y);
  void Scale3d(float x, float y, float z);
  FloatVector2d Scale2d() const {
    return FloatVector2d(matrix_.Get(0, 0), matrix_.Get(1, 1));
  }

  // Applies a scale to the current transformation and assigns the result to
  // |this|.
  void PostScale(float x, float y);

  // Applies the current transformation on a translation and assigns the result
  // to |this|.
  void Translate(const FloatVector2d& offset);
  void Translate(float x, float y);
  void Translate3d(const FloatVector3d& offset);
  void Translate3d(float x, float y, float z);

  // Applies a translation to the current transformation and assigns the result
  // to |this|.
  void PostTranslate(const FloatVector2d& offset);
  void PostTranslate(float x, float y);

  // Applies the current transformation on a skew and assigns the result
  // to |this|.
  void Skew(double angle_x, double angle_y);

  // Applies the current transformation on a perspective transform and assigns
  // the result to |this|.
  void ApplyPerspectiveDepth(float depth);

  // Applies a transformation on the current transformation
  // (i.e. 'this = this * transform;').
  void PreconcatTransform(const Transform& transform);

  // Applies a transformation on the current transformation
  // (i.e. 'this = transform * this;').
  void ConcatTransform(const Transform& transform);

  // Returns true if this is the identity matrix.
  // This function modifies a mutable variable in |matrix_|.
  bool IsIdentity() const { return matrix_.IsIdentity(); }

  // Returns true if the matrix is either identity or pure translation.
  bool IsIdentityOrTranslation() const { return matrix_.OnlyTranslate(); }

  // Returns true if the matrix is either the identity or a 2d translation.
  bool IsIdentityOr2DTranslation() const {
    return matrix_.OnlyTranslate() && matrix_.Get(2, 3) == 0;
  }

  // Returns true if the matrix is either identity or pure translation,
  // allowing for an amount of inaccuracy as specified by the parameter.
  bool IsApproximatelyIdentityOrTranslation(float tolerance) const;
  bool IsApproximatelyIdentityOrIntegerTranslation(float tolerance) const;

  // Returns true if the matrix is either a positive scale and/or a translation.
  bool IsPositiveScaleOrTranslation() const {
    if (!IsScaleOrTranslation()) return false;
    return matrix_.Get(0, 0) > 0.0 && matrix_.Get(1, 1) > 0.0 &&
           matrix_.Get(2, 2) > 0.0;
  }

  // Returns true if the matrix is identity or, if the matrix consists only
  // of a translation whose components can be represented as integers. Returns
  // false if the translation contains a fractional component or is too large to
  // fit in an integer.
  bool IsIdentityOrIntegerTranslation() const;

  // Returns true if the matrix had only scaling components.
  bool IsScale2d() const { return matrix_.OnlyScale(); }

  // Returns true if the matrix is has only scaling and translation components.
  bool IsScaleOrTranslation() const { return matrix_.OnlyScaleAndTranslate(); }

  // Returns true if axis-aligned 2d rects will remain axis-aligned after being
  // transformed by this matrix.
  bool Preserves2dAxisAlignment() const;

  // Returns true if the matrix has any perspective component that would
  // change the w-component of a homogeneous point.
  bool HasPerspective() const { return matrix_.HasPersp(); }

  // Returns true if this transform is non-singular.
  bool IsInvertible() const { return matrix_.Invert(NULL); }

  // Returns true if a layer with a forward-facing normal of (0, 0, 1) would
  // have its back side facing frontwards after applying the transform.
  bool IsBackFaceVisible() const;

  // Inverts the transform which is passed in. Returns true if successful, or
  // sets |transform| to the identify matrix on failure.
  bool GetInverse(Transform* transform) const
      __attribute__((warn_unused_result));

  // Transposes this transform in place.
  void Transpose();

  // Set 3rd row and 3rd colum to (0, 0, 1, 0). Note that this flattening
  // operation is not quite the same as an orthographic projection and is
  // technically not a linear operation.
  //
  // One useful interpretation of doing this operation:
  //  - For x and y values, the new transform behaves effectively like an
  //    orthographic projection was added to the matrix sequence.
  //  - For z values, the new transform overrides any effect that the transform
  //    had on z, and instead it preserves the z value for any points that are
  //    transformed.
  //  - Because of linearity of transforms, this flattened transform also
  //    preserves the effect that any subsequent (multiplied from the right)
  //    transforms would have on z values.
  //
  void FlattenTo2d();

  // Returns true if the 3rd row and 3rd column are both (0, 0, 1, 0).
  bool IsFlat() const;

  // Returns the x and y translation components of the matrix.
  FloatVector2d To2dTranslation() const;

  // Applies the transformation to the point.
  void TransformPoint(FloatPoint3d* point) const;

  // Applies the transformation to the point.
  void TransformPoint(FloatPoint* point) const;

  // Applies the transformation to the point.
  void TransformPoint(Point* point) const;

  // Applies the transformation to the vector.
  void TransformVector(FloatVector3d* vector) const;

  // Applies the reverse transformation on the point. Returns true if the
  // transformation can be inverted.
  bool TransformPointReverse(FloatPoint3d* point) const;

  // Applies the reverse transformation on the point. Returns true if the
  // transformation can be inverted.
  bool TransformPointReverse(FloatPoint* point) const;

  // Applies the reverse transformation on the point. Returns true if the
  // transformation can be inverted. Rounds the result to the nearest point.
  bool TransformPointReverse(Point* point) const;

  // Decomposes |this| and |from|, interpolates the decomposed values, and
  // sets |this| to the reconstituted result. Returns false if either matrix
  // can't be decomposed. Uses routines described in this spec:
  // http://www.w3.org/TR/css3-3d-transforms/.
  //
  // Note: this call is expensive since we need to decompose the transform. If
  // you're going to be calling this rapidly (e.g., in an animation) you should
  // decompose once using DecomposeTransforms and reuse your
  // DecomposedTransform.
  bool Blend(const Transform& from, double progress);

  void RoundTranslationComponents();

  // Returns |this| * |other|.
  Transform operator*(const Transform& other) const {
    return Transform(*this, other);
  }

  // Sets |this| = |this| * |other|
  Transform& operator*=(const Transform& other) {
    PreconcatTransform(other);
    return *this;
  }

  // Returns the underlying matrix.
  const skity::Matrix& matrix() const { return matrix_; }
  skity::Matrix& matrix() { return matrix_; }

  bool ApproximatelyEqual(const Transform& transform) const;

  std::string ToString() const;

 private:
  void TransformPointInternal(const skity::Matrix& xform, Point* point) const;

  void TransformPointInternal(const skity::Matrix& xform,
                              FloatPoint* point) const;

  void TransformPointInternal(const skity::Matrix& xform,
                              FloatPoint3d* point) const;

  void TransformVectorInternal(const skity::Matrix& xform,
                               FloatVector3d* vector) const;

  skity::Matrix matrix_;
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_H_
