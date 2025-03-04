// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TRANSFORMS_MATRIX44_H_
#define CORE_ANIMATION_TRANSFORMS_MATRIX44_H_

#include <array>

#include "base/include/log/logging.h"
#include "core/animation/transforms/quaternion.h"

namespace lynx {
namespace transforms {

class Matrix44 {
 public:
  constexpr Matrix44()
      : fMat{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}},
        fTypeMask(kIdentity_Mask) {}

  // The parameters are in row-major order.
  Matrix44(float col1row1, float col2row1, float col3row1, float col4row1,
           float col1row2, float col2row2, float col3row2, float col4row2,
           float col1row3, float col2row3, float col3row3, float col4row3,
           float col1row4, float col2row4, float col3row4, float col4row4)
      // fMat is indexed by [col][row] (i.e. col-major).
      : fMat{{col1row1, col1row2, col1row3, col1row4},
             {col2row1, col2row2, col2row3, col2row4},
             {col3row1, col3row2, col3row3, col3row4},
             {col4row1, col4row2, col4row3, col4row4}} {
    recomputeTypeMask();
  }

  Matrix44(const Quaternion& q)
      : Matrix44(
            // Row 0.
            1.0 - 2.0 * (q.y() * q.y() + q.z() * q.z()),
            2.0 * (q.x() * q.y() - q.z() * q.w()),
            2.0 * (q.x() * q.z() + q.y() * q.w()), 0,
            // Row 1.
            2.0 * (q.x() * q.y() + q.z() * q.w()),
            1.0 - 2.0 * (q.x() * q.x() + q.z() * q.z()),
            2.0 * (q.y() * q.z() - q.x() * q.w()), 0,
            // Row 2.
            2.0 * (q.x() * q.z() - q.y() * q.w()),
            2.0 * (q.y() * q.z() + q.x() * q.w()),
            1.0 - 2.0 * (q.x() * q.x() + q.y() * q.y()), 0,
            // Row 3.
            0, 0, 0, 1){};

  using TypeMask = uint8_t;
  enum : TypeMask {
    kIdentity_Mask = 0,
    kTranslate_Mask = 1 << 0,    //!< set if the matrix has translation
    kScale_Mask = 1 << 1,        //!< set if the matrix has any scale != 1
    kAffine_Mask = 1 << 2,       //!< set if the matrix skews or rotates
    kPerspective_Mask = 1 << 3,  //!< set if the matrix is in perspective
  };

  /**
   *  Returns a bitfield describing the transformations the matrix may
   *  perform. The bitfield is computed conservatively, so it may include
   *  false positives. For example, when kPerspective_Mask is true, all
   *  other bits may be set to true even in the case of a pure perspective
   *  transform.
   */
  inline TypeMask getType() const { return fTypeMask; }

  /**
   *  Return true if the matrix is identity.
   */
  inline bool isIdentity() const { return kIdentity_Mask == this->getType(); }

  inline bool HasPerspective() const {
    return this->getType() & kPerspective_Mask;
  }

  /**
   *  Return true if the matrix only contains scale or translate or is identity.
   */
  inline bool isScaleTranslate() const {
    return !(this->getType() & ~(kScale_Mask | kTranslate_Mask));
  }

  void setIdentity();

  /**
   *  get a value from the matrix. The row,col parameters work as follows:
   *  (0, 0)  scale-x
   *  (0, 3)  translate-x
   *  (3, 0)  perspective-x
   */
  inline float rc(int row, int col) const {
    DCHECK((unsigned)row <= 3);
    DCHECK((unsigned)col <= 3);
    return fMat[col][row];
  }

  /**
   *  set a value in the matrix. The row,col parameters work as follows:
   *  (0, 0)  scale-x
   *  (0, 3)  translate-x
   *  (3, 0)  perspective-x
   */
  inline void setRC(int row, int col, float value) {
    DCHECK((unsigned)row <= 3);
    DCHECK((unsigned)col <= 3);
    fMat[col][row] = value;
    this->recomputeTypeMask();
  }

  Matrix44& preTranslate(float dx, float dy, float dz);

  Matrix44& preScale(float sx, float sy, float sz);

  void setRotateAboutXAxis(float deg);
  void setRotateAboutYAxis(float deg);
  void setRotateAboutZAxis(float deg);

  void Skew(float angle_x, float angle_y);

  void Matrix(const std::array<float, 16>& matrix_raw_value);

  void setConcat(const Matrix44& a, const Matrix44& b);
  inline void preConcat(const Matrix44& m) { this->setConcat(*this, m); }
  inline void postConcat(const Matrix44& m) { this->setConcat(m, *this); }

  double determinant() const;
  /**
   * Provides read-only access to the underlying 4x4 matrix data.
   *
   * @return A const pointer to the first element of the matrix in a contiguous
   * memory layout. The matrix elements are stored in row-major order, suitable
   * for direct use with APIs expecting a flat array of matrix elements. The
   * caller must ensure that the Matrix44 instance outlives the use of the
   * returned pointer, as the pointer will become invalid if the Matrix44 object
   * is destroyed or modified.
   */
  const float* Data() const { return fMat[0]; }

  void mapPoint(float dst_point[2], const float src_point[2]) const;

  bool invert(Matrix44* inverse) const;

 private:
  /* This is indexed by [col][row]. */
  float fMat[4][4];
  TypeMask fTypeMask;

  static constexpr int kAllPublic_Masks = 0xF;

  float transX() const { return fMat[3][0]; }
  float transY() const { return fMat[3][1]; }
  float transZ() const { return fMat[3][2]; }

  float scaleX() const { return fMat[0][0]; }
  float scaleY() const { return fMat[1][1]; }
  float scaleZ() const { return fMat[2][2]; }

  float perspX() const { return fMat[0][3]; }
  float perspY() const { return fMat[1][3]; }
  float perspZ() const { return fMat[2][3]; }

  void recomputeTypeMask();

  inline void setTypeMask(TypeMask mask) {
    DCHECK(0 == (~kAllPublic_Masks & mask));
    fTypeMask = mask;
  }

  double invert4x4Matrix(const float in_matrix[16], float out_matrix[16]) const;
};

}  // namespace transforms
}  // namespace lynx

#endif  // CORE_ANIMATION_TRANSFORMS_MATRIX44_H_
