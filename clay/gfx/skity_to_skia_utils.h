// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_GFX_SKITY_TO_SKIA_UTILS_H_
#define CLAY_GFX_SKITY_TO_SKIA_UTILS_H_

#include "clay/gfx/image/image_info.h"
#include "skity/geometry/matrix.hpp"
#include "skity/geometry/rect.hpp"
#include "skity/geometry/rrect.hpp"
#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkClipOp.h"
#include "third_party/skia/include/core/SkM44.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"
#else
#include "skity/graphic/sampling_options.hpp"
#include "skity/render/canvas.hpp"
#endif  // ENABLE_SKITY

namespace clay {

#ifndef ENABLE_SKITY
inline skity::Matrix ConvertSkMatrixToSkityMatrix(const SkMatrix& m) {
  return skity::Matrix{m[0], m[3], 0, m[6],  //
                       m[1], m[4], 0, m[7],  //
                       0,    0,    1, 0,     //
                       m[2], m[5], 0, m[8]};
}

inline SkMatrix ConvertSkityMatrixToSkMatrix(const skity::Matrix& m) {
  SkMatrix matrix;
  matrix.setAll(m[0][0], m[1][0], m[3][0],  //
                m[0][1], m[1][1], m[3][1],  //
                m[0][3], m[1][3], m[3][3]);
  return matrix;
}

inline skity::Matrix ConvertSkM44ToMatrix(const SkM44& sk_matrix) {
  float m[16];
  sk_matrix.getColMajor(m);
  return skity::Matrix{m[0],  m[1],  m[2],  m[3],   //
                       m[4],  m[5],  m[6],  m[7],   //
                       m[8],  m[9],  m[10], m[11],  //
                       m[12], m[13], m[14], m[15]};
}

inline SkM44 ConvertSkityMatrixToSkM44(const skity::Matrix& m) {
  return SkM44{
      m[0][0], m[1][0], m[2][0], m[3][0],  //
      m[0][1], m[1][1], m[2][1], m[3][1],  //
      m[0][2], m[1][2], m[2][2], m[3][2],  //
      m[0][3], m[1][3], m[2][3], m[3][3],  //
  };
}

inline skity::Rect ConvertSkRectToSkityRect(const SkRect& sk_rect) {
  return skity::Rect(sk_rect.left(), sk_rect.top(), sk_rect.right(),
                     sk_rect.bottom());
}

inline skity::Rect ConvertSkIRectToSkityRect(const SkIRect& sk_rect) {
  return skity::Rect(sk_rect.left(), sk_rect.top(), sk_rect.right(),
                     sk_rect.bottom());
}

inline SkRect ConvertSkityRectToSkRect(const skity::Rect& sk_rect) {
  return SkRect::MakeLTRB(sk_rect.Left(), sk_rect.Top(), sk_rect.Right(),
                          sk_rect.Bottom());
}

inline SkIRect ConvertSkityRectToSkIRect(const skity::Rect& sk_rect) {
  return SkIRect::MakeLTRB(sk_rect.Left(), sk_rect.Top(), sk_rect.Right(),
                           sk_rect.Bottom());
}

inline skity::Vec2 ConvertSkVectorToSkity(const SkVector& vec) {
  return skity::Vec2{vec.fX, vec.fY};
}

inline SkVector ConvertSkityVectorToSkia(const skity::Vec2& vec) {
  return SkVector::Make(vec.x, vec.y);
}

inline skity::RRect ConvertSkRRectToSkityRRect(const SkRRect& rrect) {
  switch (rrect.getType()) {
    case SkRRect::kEmpty_Type:
      return skity::RRect::MakeEmpty();
    case SkRRect::kRect_Type:
      return skity::RRect::MakeRect(
          clay::ConvertSkRectToSkityRect(rrect.rect()));
    case SkRRect::kOval_Type:
      return skity::RRect::MakeOval(
          clay::ConvertSkRectToSkityRect(rrect.rect()));
    case SkRRect::kSimple_Type:
      return skity::RRect::MakeRectXY(
          clay::ConvertSkRectToSkityRect(rrect.rect()),
          rrect.getSimpleRadii().x(), rrect.getSimpleRadii().y());
    case SkRRect::kNinePatch_Type:
    case SkRRect::kComplex_Type:
      skity::RRect result;
      skity::Vec2 radii[4]{
          ConvertSkVectorToSkity(rrect.radii(SkRRect::kUpperLeft_Corner)),
          ConvertSkVectorToSkity(rrect.radii(SkRRect::kUpperRight_Corner)),
          ConvertSkVectorToSkity(rrect.radii(SkRRect::kLowerRight_Corner)),
          ConvertSkVectorToSkity(rrect.radii(SkRRect::kLowerLeft_Corner)),
      };
      result.SetRectRadii(clay::ConvertSkRectToSkityRect(rrect.rect()), radii);
      return result;
  }
}

inline SkRRect ConvertSkityRRectToSkia(const skity::RRect& rrect) {
  switch (rrect.GetType()) {
    case skity::RRect::Type::kEmpty:
      return SkRRect::MakeEmpty();
    case skity::RRect::Type::kRect:
      return SkRRect::MakeRect(clay::ConvertSkityRectToSkRect(rrect.GetRect()));
    case skity::RRect::Type::kOval:
      return SkRRect::MakeOval(clay::ConvertSkityRectToSkRect(rrect.GetRect()));
    case skity::RRect::Type::kSimple:
      return SkRRect::MakeRectXY(
          clay::ConvertSkityRectToSkRect(rrect.GetRect()),
          rrect.Radii(skity::RRect::Corner::kLowerLeft).x,
          rrect.Radii(skity::RRect::Corner::kLowerLeft).y);
    case skity::RRect::Type::kNinePatch:
    case skity::RRect::Type::kComplex:
      SkRRect result;
      SkVector radii[4]{
          ConvertSkityVectorToSkia(
              rrect.Radii(skity::RRect::Corner::kUpperLeft)),
          ConvertSkityVectorToSkia(
              rrect.Radii(skity::RRect::Corner::kUpperRight)),
          ConvertSkityVectorToSkia(
              rrect.Radii(skity::RRect::Corner::kLowerRight)),
          ConvertSkityVectorToSkia(
              rrect.Radii(skity::RRect::Corner::kLowerLeft)),
      };
      result.setRectRadii(clay::ConvertSkityRectToSkRect(rrect.GetRect()),
                          radii);
      return result;
  }
}
#else
inline skity::AlphaType ConvertToSkityAlphaType(AlphaType type) {
  switch (type) {
    case AlphaType::kUnknown_AlphaType:
      return skity::AlphaType::kUnknown_AlphaType;
    case AlphaType::kOpaque_AlphaType:
      return skity::AlphaType::kOpaque_AlphaType;
    case AlphaType::kPremul_AlphaType:
      return skity::AlphaType::kPremul_AlphaType;
    case AlphaType::kUnpremul_AlphaType:
      return skity::AlphaType::kUnpremul_AlphaType;
  }
}

inline skity::ColorType ConvertToSkityColorType(ColorType type) {
  switch (type) {
    case ColorType::kRGBA_8888_ColorType:
      return skity::ColorType::kRGBA;
    case ColorType::kBGRA_8888_ColorType:
      return skity::ColorType::kBGRA;
    case ColorType::kRGB_565_ColorType:
      return skity::ColorType::kRGB565;
    case ColorType::kAlpha_8_ColorType:
      return skity::ColorType::kA8;
    default:
      return skity::ColorType::kUnknown;
  }
}
inline ColorType ConvertToClayColorType(skity::ColorType skity_type) {
  switch (skity_type) {
    case skity::ColorType::kRGBA:
      return kRGBA_8888_ColorType;
    case skity::ColorType::kBGRA:
      return kBGRA_8888_ColorType;
    case skity::ColorType::kRGB565:
      return kRGB_565_ColorType;
    case skity::ColorType::kA8:
      return kAlpha_8_ColorType;
    default:
      return kUnknown_ColorType;
  }
}
inline AlphaType ConvertToClayAlphaType(skity::AlphaType skity_alpha_type) {
  switch (skity_alpha_type) {
    case skity::AlphaType::kOpaque_AlphaType:
      return kOpaque_AlphaType;
    case skity::AlphaType::kPremul_AlphaType:
      return kPremul_AlphaType;
    case skity::AlphaType::kUnpremul_AlphaType:
      return kUnpremul_AlphaType;
    default:
      return kUnknown_AlphaType;
  }
}
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_GFX_SKITY_TO_SKIA_UTILS_H_
