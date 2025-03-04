// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TRANSFORMS_TRANSFORM_OPERATION_H_
#define CORE_ANIMATION_TRANSFORMS_TRANSFORM_OPERATION_H_

#include <array>
#include <optional>

#include "core/animation/transforms/matrix44.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace tasm {
class Element;
}
namespace transforms {

struct TransformOperation {
  TransformOperation() {
    Skew();
    Scale();
    Translate();
    Rotate();
  }
  enum LengthType {
    kLengthUnit,
    kLengthPercentage,
    kLengthCalc,
  };
  enum Type {
    kIdentity = 0,
    kTranslate = 1,
    kRotateX = 1 << 2,
    kRotateY = 1 << 3,
    kRotateZ = 1 << 4,
    kScale = 1 << 5,
    kSkew = 1 << 6,
    kMatrix = 1 << 7,
    kMatrix3d = 1 << 8,
  };
  const Matrix44& GetMatrix(tasm::Element* element);

  bool NotifyElementSizeUpdated();

  bool NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

  // If you change the union value of TransformOperation, you should call Bake()
  // directly to make a new matrix!
  void Bake(tasm::Element* element);

  bool IsIdentity() const;
  static TransformOperation BlendTransformOperations(
      const TransformOperation* from, const TransformOperation* to,
      float progress, tasm::Element* element);

  struct Skew {
    Skew() {
      x = 0.0f;
      y = 0.0f;
    }
    float x, y;  // degree
  } skew;

  struct Scale {
    Scale() {
      x = 0.0f;
      y = 0.0f;
    }
    float x, y;
  } scale;

  struct Translate {
    struct Type {
      Type() {
        x = kLengthUnit;
        y = kLengthUnit;
        z = kLengthUnit;
      }
      LengthType x, y, z;
    } type;

    struct Value {
      starlight::NLength x = starlight::NLength::MakeUnitNLength(0.0f);
      starlight::NLength y = starlight::NLength::MakeUnitNLength(0.0f);
      starlight::NLength z = starlight::NLength::MakeUnitNLength(0.0f);
    } value;
    Translate() {
      type = Type();
      //      value = Value();
    }
  } translate;

  struct Rotate {
    Rotate() { degree = 0.0f; }
    float degree;
  } rotate;

  struct Matrix {
    std::array<float, 16> matrix_data = {1, 0, 0, 0,   // {{1, 0, 0, 0}
                                         0, 1, 0, 0,   //  {0, 1, 0, 0}
                                         0, 0, 1, 0,   //  {0, 0, 1, 0}
                                         0, 0, 0, 1};  //  {0, 0, 0, 1}}
  } matrix;

  Type type = kIdentity;

  tasm::CSSValuePattern unit_type_0_ = tasm::CSSValuePattern::EMPTY;
  tasm::CSSValuePattern unit_type_1_ = tasm::CSSValuePattern::EMPTY;
  tasm::CSSValuePattern unit_type_2_ = tasm::CSSValuePattern::EMPTY;

 private:
  // matrix44 is just a cache of intermediate results that are computed.
  std::optional<Matrix44> matrix44;
};
}  // namespace transforms
}  // namespace lynx

#endif  // CORE_ANIMATION_TRANSFORMS_TRANSFORM_OPERATION_H_
