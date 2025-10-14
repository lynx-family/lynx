// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_complexity.h"
#include "clay/flow/layers/picture_complexity_gl.h"
#include "clay/flow/layers/picture_complexity_metal.h"
#include "clay/flow/testing/picture_test_utils.h"
#include "clay/gfx/testing_utils.h"
#include "clay/testing/testing.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRSXform.h"

namespace clay {
namespace testing {

namespace {

std::vector<PictureComplexityCalculator*> Calculators() {
  return {PictureMetalComplexityCalculator::GetInstance(),
          PictureGLComplexityCalculator::GetInstance(),
          PictureNaiveComplexityCalculator::GetInstance()};
}

std::vector<PictureComplexityCalculator*> AccumulatorCalculators() {
  return {PictureMetalComplexityCalculator::GetInstance(),
          PictureGLComplexityCalculator::GetInstance()};
}

std::vector<SkPoint> GetTestPoints() {
  std::vector<SkPoint> points;
  points.push_back(SkPoint::Make(0, 0));
  points.push_back(SkPoint::Make(10, 0));
  points.push_back(SkPoint::Make(10, 10));
  points.push_back(SkPoint::Make(20, 10));
  points.push_back(SkPoint::Make(20, 20));

  return points;
}

}  // namespace

TEST(DisplayListComplexity, EmptyDisplayList) {
  auto display_list = GetSamplePicture(0);

  auto calculators = Calculators();
  for (auto calculator : calculators) {
    ASSERT_EQ(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DisplayListCeiling) {
  auto display_list = GetSamplePicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    calculator->SetComplexityCeiling(10u);
    ASSERT_EQ(calculator->Compute(display_list.get()), 10u);
    calculator->SetComplexityCeiling(std::numeric_limits<unsigned int>::max());
  }
}

TEST(DisplayListComplexity, NestedDisplayList) {
  auto display_list = GetSampleNestedPicture();

  auto calculators = Calculators();
  for (auto calculator : calculators) {
    // There's only one draw call in the "outer" DisplayList, which calls
    // drawDisplayList with the "inner" DisplayList. To ensure we are
    // recursing correctly into the inner DisplayList, check that we aren't
    // returning 0 (if the function is a no-op) or 1 (as the op_count is 1)
    ASSERT_GT(calculator->Compute(display_list.get()), 1u);
  }
}

TEST(DisplayListComplexity, Style) {
  SkPictureRecorder recorder_filled;
  auto canvas_filled =
      recorder_filled.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  SkPaint paint;
  paint.setStyle(SkPaint::kFill_Style);
  canvas_filled->drawRect(SkRect::MakeXYWH(10, 10, 80, 80), paint);
  auto display_list_filled = recorder_filled.finishRecordingAsPicture();

  SkPictureRecorder recorder_stroked;
  auto canvas_stroked =
      recorder_stroked.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  paint.setStyle(SkPaint::kStroke_Style);
  canvas_stroked->drawRect(SkRect::MakeXYWH(10, 10, 80, 80), paint);
  auto display_list_stroked = recorder_stroked.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list_filled.get()),
              calculator->Compute(display_list_stroked.get()));
  }
}

TEST(DisplayListComplexity, DrawPath) {
  SkPath line_path;
  line_path.moveTo(SkPoint::Make(0, 0));
  line_path.lineTo(SkPoint::Make(10, 10));
  line_path.close();

  SkPath quad_path;
  quad_path.moveTo(SkPoint::Make(0, 0));
  quad_path.quadTo(SkPoint::Make(10, 10), SkPoint::Make(10, 20));
  quad_path.close();

  SkPath conic_path;
  conic_path.moveTo(SkPoint::Make(0, 0));
  conic_path.conicTo(SkPoint::Make(10, 10), SkPoint::Make(10, 20), 1.5f);
  conic_path.close();

  SkPath cubic_path;
  cubic_path.moveTo(SkPoint::Make(0, 0));
  cubic_path.cubicTo(SkPoint::Make(10, 10), SkPoint::Make(10, 20),
                     SkPoint::Make(20, 20));
  cubic_path.close();

  SkPictureRecorder recorder_line;
  auto canvas_line =
      recorder_line.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_line->drawPath(line_path, SkPaint());
  auto display_list_line = recorder_line.finishRecordingAsPicture();

  SkPictureRecorder recorder_quad;
  auto canvas_quad =
      recorder_quad.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_quad->drawPath(quad_path, SkPaint());
  auto display_list_quad = recorder_quad.finishRecordingAsPicture();

  SkPictureRecorder recorder_conic;
  auto canvas_conic =
      recorder_conic.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_conic->drawPath(conic_path, SkPaint());
  auto display_list_conic = recorder_conic.finishRecordingAsPicture();

  SkPictureRecorder recorder_cubic;
  auto canvas_cubic =
      recorder_cubic.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_cubic->drawPath(cubic_path, SkPaint());
  auto display_list_cubic = recorder_cubic.finishRecordingAsPicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list_line.get()), 0u);
    ASSERT_NE(calculator->Compute(display_list_quad.get()), 0u);
    ASSERT_NE(calculator->Compute(display_list_conic.get()), 0u);
    ASSERT_NE(calculator->Compute(display_list_cubic.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawOval) {
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawOval(SkRect::MakeXYWH(10, 10, 100, 80), SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawCircle) {
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawCircle(SkPoint::Make(50, 50), 10.0f, SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawRRect) {
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawRRect(
      SkRRect::MakeRectXY(SkRect::MakeXYWH(10, 10, 80, 80), 2.0f, 3.0f),
      SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawDRRect) {
  SkRRect outer =
      SkRRect::MakeRectXY(SkRect::MakeXYWH(10, 10, 80, 80), 2.0f, 3.0f);
  SkRRect inner =
      SkRRect::MakeRectXY(SkRect::MakeXYWH(15, 15, 70, 70), 1.5f, 1.5f);
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawDRRect(outer, inner, SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawArc) {
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawArc(SkRect::MakeXYWH(10, 10, 100, 80), 0.0f, 10.0f, true,
                  SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawVertices) {
  auto points = GetTestPoints();
  auto vertices = DlVertices::Make(DlVertexMode::kTriangles, points.size(),
                                   points.data(), nullptr, nullptr);
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawVertices(vertices->gr_object(), SkBlendMode::kSrc, SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawTextBlob) {
  auto text_blob = SkTextBlob::MakeFromString(
      "The quick brown fox jumps over the lazy dog.", SkFont());
  auto dl_text_blob = DlTextBlob::Make(text_blob);
  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawTextBlob(dl_text_blob->gr_text_blob(), 0.0f, 0.0f, SkPaint());
  auto display_list = recorder.finishRecordingAsPicture();

  SkPictureRecorder recorder_multiple;
  auto canvas_multiple =
      recorder_multiple.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_multiple->drawTextBlob(dl_text_blob->gr_text_blob(), 0.0f, 0.0f,
                                SkPaint());
  canvas_multiple->drawTextBlob(dl_text_blob->gr_text_blob(), 0.0f, 0.0f,
                                SkPaint());
  auto display_list_multiple = recorder_multiple.finishRecordingAsPicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
    ASSERT_GT(calculator->Compute(display_list_multiple.get()),
              calculator->Compute(display_list.get()));
  }
}

TEST(DisplayListComplexity, DrawPoints) {
  auto points = GetTestPoints();
  SkPictureRecorder recorder_lines;
  auto canvas_lines =
      recorder_lines.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_lines->drawPoints(SkCanvas::kLines_PointMode, points.size(),
                           points.data(), SkPaint());
  auto display_list_lines = recorder_lines.finishRecordingAsPicture();

  SkPictureRecorder recorder_points;
  auto canvas_points =
      recorder_points.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_points->drawPoints(SkCanvas::kPoints_PointMode, points.size(),
                            points.data(), SkPaint());
  auto display_list_points = recorder_points.finishRecordingAsPicture();

  SkPictureRecorder recorder_polygon;
  auto canvas_polygon =
      recorder_polygon.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas_polygon->drawPoints(SkCanvas::kPolygon_PointMode, points.size(),
                             points.data(), SkPaint());
  auto display_list_polygon = recorder_polygon.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list_lines.get()), 0u);
    ASSERT_NE(calculator->Compute(display_list_points.get()), 0u);
    ASSERT_NE(calculator->Compute(display_list_polygon.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawImage) {
  SkImageInfo info =
      SkImageInfo::Make(50, 50, SkColorType::kRGBA_8888_SkColorType,
                        SkAlphaType::kPremul_SkAlphaType);
  SkBitmap bitmap;
  bitmap.allocPixels(info, 0);
  auto image = SkImages::RasterFromBitmap(bitmap);

  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawImage(image, 0, 0);
  auto display_list = recorder.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawImageNine) {
  SkImageInfo info =
      SkImageInfo::Make(50, 50, SkColorType::kRGBA_8888_SkColorType,
                        SkAlphaType::kPremul_SkAlphaType);
  SkBitmap bitmap;
  bitmap.allocPixels(info, 0);
  auto image = SkImages::RasterFromBitmap(bitmap);

  SkIRect center = SkIRect::MakeXYWH(5, 5, 20, 20);
  SkRect dest = SkRect::MakeXYWH(0, 0, 50, 50);

  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawImageNine(image.get(), center, dest, SkFilterMode::kNearest);
  auto display_list = recorder.finishRecordingAsPicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawImageRect) {
  SkImageInfo info =
      SkImageInfo::Make(50, 50, SkColorType::kRGBA_8888_SkColorType,
                        SkAlphaType::kPremul_SkAlphaType);
  SkBitmap bitmap;
  bitmap.allocPixels(info, 0);
  auto image = SkImages::RasterFromBitmap(bitmap);

  SkRect src = SkRect::MakeXYWH(0, 0, 50, 50);
  SkRect dest = SkRect::MakeXYWH(0, 0, 50, 50);

  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  SkPaint paint;
  canvas->drawImageRect(image, src, dest, {}, &paint,
                        SkCanvas::SrcRectConstraint::kFast_SrcRectConstraint);
  auto display_list = recorder.finishRecordingAsPicture();

  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

TEST(DisplayListComplexity, DrawAtlas) {
  SkImageInfo info =
      SkImageInfo::Make(50, 50, SkColorType::kRGBA_8888_SkColorType,
                        SkAlphaType::kPremul_SkAlphaType);
  SkBitmap bitmap;
  bitmap.allocPixels(info, 0);
  auto image = SkImages::RasterFromBitmap(bitmap);

  std::vector<SkRect> rects;
  std::vector<SkRSXform> xforms;
  for (int i = 0; i < 10; i++) {
    rects.push_back(SkRect::MakeXYWH(0, 0, 10, 10));
    xforms.push_back(SkRSXform::Make(0, 0, 0, 0));
  }

  SkPictureRecorder recorder;
  auto canvas = recorder.beginRecording(SkRect::MakeLTRB(0, 0, 100, 100));
  canvas->drawAtlas(image.get(), xforms.data(), rects.data(), nullptr, 10,
                    SkBlendMode::kSrc, {}, nullptr, nullptr);
  auto display_list = recorder.finishRecordingAsPicture();
  auto calculators = AccumulatorCalculators();
  for (auto calculator : calculators) {
    ASSERT_NE(calculator->Compute(display_list.get()), 0u);
  }
}

}  // namespace testing
}  // namespace clay
