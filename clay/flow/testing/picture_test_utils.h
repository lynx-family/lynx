// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_TESTING_PICTURE_TEST_UTILS_H_
#define CLAY_FLOW_TESTING_PICTURE_TEST_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "clay/gfx/testing_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/effects/SkDashPathEffect.h"
#include "third_party/skia/include/effects/SkGradientShader.h"
#include "third_party/skia/include/effects/SkImageFilters.h"

namespace clay {
namespace testing {

sk_sp<SkPicture> GetSamplePicture();
sk_sp<SkPicture> GetSamplePicture(int ops);
sk_sp<SkPicture> GetSampleNestedPicture();

constexpr skity::Vec2 kEndPoints[] = {
    {0, 0},
    {100, 100},
};
const clay::Color kColors[] = {
    clay::Color::kGreen(),
    clay::Color::kYellow(),
    clay::Color::kBlue(),
};
constexpr float kStops[] = {
    0.0,
    0.5,
    1.0,
};
static std::vector<uint32_t> color_vector(kColors, kColors + 3);
static std::vector<float> stops_vector(kStops, kStops + 3);

// clang-format off
constexpr float kRotateColorMatrix[20] = {
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    1, 0, 0, 0, 0,
    0, 0, 0, 1, 0,
};
constexpr float kInvertColorMatrix[20] = {
    -1.0,    0,    0, 1.0,   0,
       0, -1.0,    0, 1.0,   0,
       0,    0, -1.0, 1.0,   0,
     1.0,  1.0,  1.0, 1.0,   0,
};
// clang-format on

const SkScalar kTestDashes1[] = {4.0, 2.0};
const SkScalar kTestDashes2[] = {1.0, 1.5};

constexpr SkPoint TestPoints[] = {
    {10, 10},
    {20, 20},
    {10, 20},
    {20, 10},
};
#define TestPointCount sizeof(TestPoints) / (sizeof(TestPoints[0]))

static clay::ImageSampling kNearestSampling =
    clay::ImageSampling::kNearestNeighbor;
static clay::ImageSampling kLinearSampling = clay::ImageSampling::kLinear;

static fml::RefPtr<DlImage> MakeTestImage(int w, int h, int checker_size) {
  sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(w, h);
  SkCanvas* canvas = surface->getCanvas();
  SkPaint p0, p1;
  p0.setStyle(SkPaint::kFill_Style);
  p0.setColor(SK_ColorGREEN);
  p1.setStyle(SkPaint::kFill_Style);
  p1.setColor(SK_ColorBLUE);
  p1.setAlpha(128);
  for (int y = 0; y < w; y += checker_size) {
    for (int x = 0; x < h; x += checker_size) {
      SkPaint& cellp = ((x + y) & 1) == 0 ? p0 : p1;
      canvas->drawRect(SkRect::MakeXYWH(x, y, checker_size, checker_size),
                       cellp);
    }
  }
  return DlImage::Make(surface->makeImageSnapshot());
}

static auto TestImage1 = MakeTestImage(40, 40, 5);
static auto TestImage2 = MakeTestImage(50, 50, 5);

static const clay::ImageColorSource kTestSource1(TestImage1,
                                                 clay::TileMode::kClamp,
                                                 clay::TileMode::kMirror,
                                                 kLinearSampling);
static const std::shared_ptr<clay::ColorSource> kTestSource2 =
    clay::ColorSource::MakeLinear(kEndPoints[0], kEndPoints[1], 3, kColors,
                                  kStops, clay::TileMode::kMirror);
static const std::shared_ptr<clay::ColorSource> kTestSource3 =
    clay::ColorSource::MakeRadial(kEndPoints[0], 10.0, 3, kColors, kStops,
                                  clay::TileMode::kMirror);
static const std::shared_ptr<clay::ColorSource> kTestSource4 =
    clay::ColorSource::MakeConical(kEndPoints[0], 10.0, kEndPoints[1], 200.0, 3,
                                   kColors, kStops, clay::TileMode::kDecal);
static const std::shared_ptr<clay::ColorSource> kTestSource5 =
    clay::ColorSource::MakeSweep(kEndPoints[0], 0.0, 360.0, 3, kColors, kStops,
                                 clay::TileMode::kDecal);
static const clay::BlendColorFilter kTestBlendColorFilter1(
    clay::Color::kRed(), clay::BlendMode::kDstATop);
static const clay::BlendColorFilter kTestBlendColorFilter2(
    clay::Color::kBlue(), clay::BlendMode::kDstATop);
static const clay::BlendColorFilter kTestBlendColorFilter3(
    clay::Color::kRed(), clay::BlendMode::kDstIn);
static const clay::MatrixColorFilter kTestMatrixColorFilter1(
    kRotateColorMatrix);
static const clay::MatrixColorFilter kTestMatrixColorFilter2(
    kInvertColorMatrix);
static const clay::BlurImageFilter kTestBlurImageFilter1(
    5.0, 5.0, clay::TileMode::kClamp);
static const clay::BlurImageFilter kTestBlurImageFilter2(
    6.0, 5.0, clay::TileMode::kClamp);
static const clay::BlurImageFilter kTestBlurImageFilter3(
    5.0, 6.0, clay::TileMode::kClamp);
static const clay::BlurImageFilter kTestBlurImageFilter4(
    5.0, 5.0, clay::TileMode::kDecal);
static const clay::DilateImageFilter kTestDilateImageFilter1(5.0, 5.0);
static const clay::DilateImageFilter kTestDilateImageFilter2(6.0, 5.0);
static const clay::DilateImageFilter kTestDilateImageFilter3(5.0, 6.0);
static const clay::ErodeImageFilter kTestErodeImageFilter1(5.0, 5.0);
static const clay::ErodeImageFilter kTestErodeImageFilter2(6.0, 5.0);
static const clay::ErodeImageFilter kTestErodeImageFilter3(5.0, 6.0);
static const clay::MatrixImageFilter kTestMatrixImageFilter1(
    skity::Matrix::RotateDeg(45), kNearestSampling);
static const clay::MatrixImageFilter kTestMatrixImageFilter2(
    skity::Matrix::RotateDeg(85), kNearestSampling);
static const clay::MatrixImageFilter kTestMatrixImageFilter3(
    skity::Matrix::RotateDeg(45), kLinearSampling);
static const clay::ComposeImageFilter kTestComposeImageFilter1(
    kTestBlurImageFilter1, kTestMatrixImageFilter1);
static const clay::ComposeImageFilter kTestComposeImageFilter2(
    kTestBlurImageFilter2, kTestMatrixImageFilter1);
static const clay::ComposeImageFilter kTestComposeImageFilter3(
    kTestBlurImageFilter1, kTestMatrixImageFilter2);
static const clay::ColorFilterImageFilter kTestCFImageFilter1(
    kTestBlendColorFilter1);
static const clay::ColorFilterImageFilter kTestCFImageFilter2(
    kTestBlendColorFilter2);
static const std::shared_ptr<clay::PathEffect> kTestPathEffect1 =
    clay::DashPathEffect::Make(kTestDashes1, 2, 0.0f);
static const std::shared_ptr<clay::PathEffect> kTestPathEffect2 =
    clay::DashPathEffect::Make(kTestDashes2, 2, 0.0f);
static const clay::BlurMaskFilter kTestMaskFilter1(kNormal_SkBlurStyle, 3.0);
static const clay::BlurMaskFilter kTestMaskFilter2(kNormal_SkBlurStyle, 5.0);
static const clay::BlurMaskFilter kTestMaskFilter3(kSolid_SkBlurStyle, 3.0);
static const clay::BlurMaskFilter kTestMaskFilter4(kInner_SkBlurStyle, 3.0);
static const clay::BlurMaskFilter kTestMaskFilter5(kOuter_SkBlurStyle, 3.0);
constexpr SkRect kTestBounds = SkRect::MakeLTRB(10, 10, 50, 60);
static const SkRRect kTestRRect = SkRRect::MakeRectXY(kTestBounds, 5, 5);
static const SkRRect kTestRRectRect = SkRRect::MakeRect(kTestBounds);
static const SkRRect kTestInnerRRect =
    SkRRect::MakeRectXY(kTestBounds.makeInset(5, 5), 2, 2);
static const SkPath kTestPathRect = SkPath::Rect(kTestBounds);
static const SkPath kTestPathOval = SkPath::Oval(kTestBounds);
static const SkPath kTestPath1 =
    SkPath::Polygon({{0, 0}, {10, 10}, {10, 0}, {0, 10}}, true);
static const SkPath kTestPath2 =
    SkPath::Polygon({{0, 0}, {10, 10}, {0, 10}, {10, 0}}, true);
static const SkPath kTestPath3 =
    SkPath::Polygon({{0, 0}, {10, 10}, {10, 0}, {0, 10}}, false);
static const SkMatrix kTestMatrix1 = SkMatrix::Scale(2, 2);
static const SkMatrix kTestMatrix2 = SkMatrix::RotateDeg(45);

static std::shared_ptr<const DlVertices> TestVertices1 =
    DlVertices::Make(DlVertexMode::kTriangles,  //
                     3, TestPoints, nullptr, kColors);
static std::shared_ptr<const DlVertices> TestVertices2 =
    DlVertices::Make(DlVertexMode::kTriangleFan,  //
                     3, TestPoints, nullptr, kColors);

static constexpr int kTestDivs1[] = {10, 20, 30};
static constexpr int kTestDivs2[] = {15, 20, 25};
static constexpr int kTestDivs3[] = {15, 25};
static constexpr SkCanvas::Lattice::RectType kTestRTypes[] = {
    SkCanvas::Lattice::RectType::kDefault,
    SkCanvas::Lattice::RectType::kTransparent,
    SkCanvas::Lattice::RectType::kFixedColor,
    SkCanvas::Lattice::RectType::kDefault,
    SkCanvas::Lattice::RectType::kTransparent,
    SkCanvas::Lattice::RectType::kFixedColor,
    SkCanvas::Lattice::RectType::kDefault,
    SkCanvas::Lattice::RectType::kTransparent,
    SkCanvas::Lattice::RectType::kFixedColor,
};
static constexpr SkColor kTestLatticeColors[] = {
    SK_ColorBLUE, SK_ColorGREEN, SK_ColorYELLOW,
    SK_ColorBLUE, SK_ColorGREEN, SK_ColorYELLOW,
    SK_ColorBLUE, SK_ColorGREEN, SK_ColorYELLOW,
};
static constexpr SkIRect kTestLatticeSrcRect = {1, 1, 39, 39};

static sk_sp<SkPicture> MakeTestPicture(int w, int h, SkColor color) {
  SkPictureRecorder recorder;
  SkRTreeFactory rtree_factory;
  SkCanvas* cv = recorder.beginRecording(kTestBounds, &rtree_factory);
  SkPaint paint;
  paint.setColor(color);
  paint.setStyle(SkPaint::kFill_Style);
  cv->drawRect(SkRect::MakeWH(w, h), paint);
  return recorder.finishRecordingAsPicture();
}
static sk_sp<SkPicture> TestPicture1 = MakeTestPicture(20, 20, SK_ColorGREEN);
static sk_sp<SkPicture> TestPicture2 = MakeTestPicture(25, 25, SK_ColorBLUE);

static sk_sp<SkTextBlob> MakeTextBlob(std::string string) {
  return SkTextBlob::MakeFromText(string.c_str(), string.size(), SkFont(),
                                  SkTextEncoding::kUTF8);
}
static sk_sp<SkTextBlob> TestBlob1 = MakeTextBlob("TestBlob1");
static sk_sp<SkTextBlob> TestBlob2 = MakeTextBlob("TestBlob2");
}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_PICTURE_TEST_UTILS_H_
