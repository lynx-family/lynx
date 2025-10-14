// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint.h"
#include "clay/gfx/testing_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"

namespace clay {
namespace testing {

TEST(DisplayListEnum, ToDlTileMode) {
  ASSERT_EQ(clay::ToClay(SkTileMode::kClamp), DlTileMode::kClamp);
  ASSERT_EQ(clay::ToClay(SkTileMode::kRepeat), DlTileMode::kRepeat);
  ASSERT_EQ(clay::ToClay(SkTileMode::kMirror), DlTileMode::kMirror);
  ASSERT_EQ(clay::ToClay(SkTileMode::kDecal), DlTileMode::kDecal);
}

TEST(DisplayListEnum, ToSkTileMode) {
  ASSERT_EQ(ToSk(DlTileMode::kClamp), SkTileMode::kClamp);
  ASSERT_EQ(ToSk(DlTileMode::kRepeat), SkTileMode::kRepeat);
  ASSERT_EQ(ToSk(DlTileMode::kMirror), SkTileMode::kMirror);
  ASSERT_EQ(ToSk(DlTileMode::kDecal), SkTileMode::kDecal);
}

TEST(DisplayListEnum, ToDlDrawStyle) {
  ASSERT_EQ(clay::ToClay(SkPaint::Style::kFill_Style), DlDrawStyle::kFill);
  ASSERT_EQ(clay::ToClay(SkPaint::Style::kStroke_Style), DlDrawStyle::kStroke);
  ASSERT_EQ(clay::ToClay(SkPaint::Style::kStrokeAndFill_Style),
            DlDrawStyle::kStrokeAndFill);
}

TEST(DisplayListEnum, ToSkDrawStyle) {
  ASSERT_EQ(ToSk(DlDrawStyle::kFill), SkPaint::Style::kFill_Style);
  ASSERT_EQ(ToSk(DlDrawStyle::kStroke), SkPaint::Style::kStroke_Style);
  ASSERT_EQ(ToSk(DlDrawStyle::kStrokeAndFill),
            SkPaint::Style::kStrokeAndFill_Style);
}

TEST(DisplayListEnum, ToDlStrokeCap) {
  ASSERT_EQ(clay::ToClay(SkPaint::Cap::kButt_Cap), DlStrokeCap::kButt);
  ASSERT_EQ(clay::ToClay(SkPaint::Cap::kRound_Cap), DlStrokeCap::kRound);
  ASSERT_EQ(clay::ToClay(SkPaint::Cap::kSquare_Cap), DlStrokeCap::kSquare);
}

TEST(DisplayListEnum, ToSkStrokeCap) {
  ASSERT_EQ(ToSk(DlStrokeCap::kButt), SkPaint::Cap::kButt_Cap);
  ASSERT_EQ(ToSk(DlStrokeCap::kRound), SkPaint::Cap::kRound_Cap);
  ASSERT_EQ(ToSk(DlStrokeCap::kSquare), SkPaint::Cap::kSquare_Cap);
}

TEST(DisplayListEnum, ToDlStrokeJoin) {
  ASSERT_EQ(clay::ToClay(SkPaint::Join::kMiter_Join), DlStrokeJoin::kMiter);
  ASSERT_EQ(clay::ToClay(SkPaint::Join::kRound_Join), DlStrokeJoin::kRound);
  ASSERT_EQ(clay::ToClay(SkPaint::Join::kBevel_Join), DlStrokeJoin::kBevel);
}

TEST(DisplayListEnum, ToSkStrokeJoin) {
  ASSERT_EQ(ToSk(DlStrokeJoin::kMiter), SkPaint::Join::kMiter_Join);
  ASSERT_EQ(ToSk(DlStrokeJoin::kRound), SkPaint::Join::kRound_Join);
  ASSERT_EQ(ToSk(DlStrokeJoin::kBevel), SkPaint::Join::kBevel_Join);
}

TEST(DisplayListEnum, ToDlVertexMode) {
  ASSERT_EQ(clay::ToClay(SkVertices::VertexMode::kTriangles_VertexMode),
            DlVertexMode::kTriangles);
  ASSERT_EQ(clay::ToClay(SkVertices::VertexMode::kTriangleStrip_VertexMode),
            DlVertexMode::kTriangleStrip);
  ASSERT_EQ(clay::ToClay(SkVertices::VertexMode::kTriangleFan_VertexMode),
            DlVertexMode::kTriangleFan);
}

TEST(DisplayListEnum, ToSkVertexMode) {
  ASSERT_EQ(ToSk(DlVertexMode::kTriangles),
            SkVertices::VertexMode::kTriangles_VertexMode);
  ASSERT_EQ(ToSk(DlVertexMode::kTriangleStrip),
            SkVertices::VertexMode::kTriangleStrip_VertexMode);
  ASSERT_EQ(ToSk(DlVertexMode::kTriangleFan),
            SkVertices::VertexMode::kTriangleFan_VertexMode);
}

TEST(DisplayListEnum, ToDlFilterMode) {
  ASSERT_EQ(clay::ToClay(SkFilterMode::kLinear), DlFilterMode::kLinear);
  ASSERT_EQ(clay::ToClay(SkFilterMode::kNearest), DlFilterMode::kNearest);
  ASSERT_EQ(clay::ToClay(SkFilterMode::kLast), DlFilterMode::kLast);
}

TEST(DisplayListEnum, ToSkFilterMode) {
  ASSERT_EQ(ToSk(DlFilterMode::kLinear), SkFilterMode::kLinear);
  ASSERT_EQ(ToSk(DlFilterMode::kNearest), SkFilterMode::kNearest);
  ASSERT_EQ(ToSk(DlFilterMode::kLast), SkFilterMode::kLast);
}

TEST(DisplayListEnum, ToDlImageSampling) {
  ASSERT_EQ(clay::ToClay(
                SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone)),
            DlImageSampling::kLinear);
  ASSERT_EQ(clay::ToClay(SkSamplingOptions(SkFilterMode::kLinear,
                                           SkMipmapMode::kLinear)),
            DlImageSampling::kMipmapLinear);
  ASSERT_EQ(clay::ToClay(
                SkSamplingOptions(SkFilterMode::kNearest, SkMipmapMode::kNone)),
            DlImageSampling::kNearestNeighbor);
  ASSERT_EQ(
      clay::ToClay(SkSamplingOptions(SkCubicResampler{1 / 3.0f, 1 / 3.0f})),
      DlImageSampling::kCubic);
}

TEST(DisplayListEnum, ToSkSamplingOptions) {
  ASSERT_EQ(ToSk(DlImageSampling::kLinear),
            SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone));
  ASSERT_EQ(ToSk(DlImageSampling::kMipmapLinear),
            SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear));
  ASSERT_EQ(ToSk(DlImageSampling::kNearestNeighbor),
            SkSamplingOptions(SkFilterMode::kNearest, SkMipmapMode::kNone));
  ASSERT_EQ(ToSk(DlImageSampling::kCubic),
            SkSamplingOptions(SkCubicResampler{1 / 3.0f, 1 / 3.0f}));
}

#define CHECK_TO_DLENUM(V) \
  ASSERT_EQ(clay::ToClay(SkBlendMode::V), DlBlendMode::V);
#define CHECK_TO_SKENUM(V) ASSERT_EQ(ToSk(DlBlendMode::V), SkBlendMode::V);

#define FOR_EACH_ENUM(FUNC) \
  FUNC(kSrc)                \
  FUNC(kClear)              \
  FUNC(kSrc)                \
  FUNC(kDst)                \
  FUNC(kSrcOver)            \
  FUNC(kDstOver)            \
  FUNC(kSrcIn)              \
  FUNC(kDstIn)              \
  FUNC(kSrcOut)             \
  FUNC(kDstOut)             \
  FUNC(kSrcATop)            \
  FUNC(kDstATop)            \
  FUNC(kXor)                \
  FUNC(kPlus)               \
  FUNC(kModulate)           \
  FUNC(kScreen)             \
  FUNC(kOverlay)            \
  FUNC(kDarken)             \
  FUNC(kLighten)            \
  FUNC(kColorDodge)         \
  FUNC(kColorBurn)          \
  FUNC(kHardLight)          \
  FUNC(kSoftLight)          \
  FUNC(kDifference)         \
  FUNC(kExclusion)          \
  FUNC(kMultiply)           \
  FUNC(kHue)                \
  FUNC(kSaturation)         \
  FUNC(kColor)              \
  FUNC(kLuminosity)         \
  FUNC(kLastCoeffMode)      \
  FUNC(kLastSeparableMode)  \
  FUNC(kLastMode)

TEST(DisplayListEnum, ToDlBlendMode){FOR_EACH_ENUM(CHECK_TO_DLENUM)}

TEST(DisplayListEnum, ToSkBlendMode) {
  FOR_EACH_ENUM(CHECK_TO_SKENUM)
}

#undef CHECK_TO_DLENUM
#undef CHECK_TO_SKENUM
#undef FOR_EACH_ENUM

}  // namespace testing
}  // namespace clay
