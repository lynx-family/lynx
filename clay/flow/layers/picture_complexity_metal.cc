// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_complexity_metal.h"

#include <cmath>

// The numbers and weightings used in this file stem from taking the
// data from the DisplayListBenchmarks suite run on an iPhone 12 and
// applying very rough analysis on them to identify the approximate
// trends.
//
// See the comments in display_list_complexity_helper.h for details on the
// process and rationale behind coming up with these numbers.

namespace clay {

PictureMetalComplexityCalculator* PictureMetalComplexityCalculator::instance_ =
    nullptr;

PictureMetalComplexityCalculator*
PictureMetalComplexityCalculator::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new PictureMetalComplexityCalculator();
  }
  return instance_;
}

#ifndef ENABLE_SKITY
unsigned int
PictureMetalComplexityCalculator::MetalHelper::BatchedComplexity() {
  // Calculate the impact of saveLayer.
  unsigned int save_layer_complexity;
  if (save_layer_count_ == 0) {
    save_layer_complexity = 0;
  } else {
    // saveLayer seems to have two trends; if the count is < 200,
    // then the individual cost of a saveLayer is higher than if
    // the count is > 200.
    //
    // However, the trend is strange and we should gather more data to
    // get a better idea of how to represent the trend. That being said, it's
    // very unlikely we'll ever hit a DisplayList with 200+ saveLayer calls
    // in it, so we will calculate based on the more reasonably anticipated
    // range of less than 200, with the trend line more weighted towards the
    // lower end of that range (as the data itself doesn't present as a straight
    // line). Further, we will easily hit our cache thresholds with such a
    // large number of saveLayer calls.
    //
    // m = 1/2
    // c = 1
    save_layer_complexity = (save_layer_count_ + 2) * 100000;
  }

  unsigned int draw_text_blob_complexity;
  if (draw_text_blob_count_ == 0) {
    draw_text_blob_complexity = 0;
  } else {
    // m = 1/240
    // c = 0.75
    draw_text_blob_complexity = (draw_text_blob_count_ + 180) * 2500 / 3;
  }

  return save_layer_complexity + draw_text_blob_complexity;
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawRect(
    const SkRect& rect, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  unsigned int complexity;

  // If stroked, cost scales linearly with the rectangle width/height.
  // If filled, it scales with the area.
  //
  // Hairline stroke vs non hairline has no real penalty at smaller lengths,
  // but it increases at larger lengths. There isn't enough data to get a good
  // idea of the penalty at lengths > 1000px.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style() == SkPaint::Style::kFill_Style) {
    // No real difference for AA with filled styles.
    unsigned int area = rect.width() * rect.height();

    // m = 1/9000
    // c = 0
    complexity = area / 225;
  } else {
    // Take the average of the width and height.
    unsigned int length = (rect.width() + rect.height()) / 2;

    // There is a penalty for AA being *disabled*.
    if (IsAntiAliased()) {
      // m = 1/65
      // c = 0
      complexity = length * 8 / 13;
    } else {
      // m = 1/35
      // c = 0
      complexity = length * 8 / 7;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawOval(
    const SkRect& bounds, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // DrawOval scales very roughly linearly with the bounding box width/height
  // (not area) for stroked styles without AA.
  //
  // Filled styles and stroked styles with AA scale linearly with the bounding
  // box area.
  unsigned int area = bounds.width() * bounds.height();

  unsigned int complexity;

  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style() == SkPaint::Style::kFill_Style) {
    // With filled styles, there is no significant AA penalty.
    // m = 1/16000
    // c = 0
    complexity = area / 80;
  } else {
    if (IsAntiAliased()) {
      // m = 1/7500
      // c = 0
      complexity = area * 2 / 75;
    } else {
      // Take the average of the width and height.
      unsigned int length = (bounds.width() + bounds.height()) / 2;

      // m = 1/80
      // c = 0
      complexity = length * 5 / 2;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawRRect(
    const SkRRect& rrect, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // RRects scale linearly with the area of the bounding rect.
  unsigned int area = rrect.width() * rrect.height();

  // Drawing RRects is split into two performance tiers; an expensive
  // one and a less expensive one. Both scale linearly with area.
  //
  // Expensive: All filled style, symmetric w/AA.
  bool expensive =
      (Style() == SkPaint::Style::kFill_Style) ||
      ((rrect.getType() == SkRRect::Type::kSimple_Type) && IsAntiAliased());

  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  if (expensive) {
    // m = 1/25000
    // c = 2
    // An area of 7000px^2 ~= baseline timing of 0.0005ms.
    complexity = (area + 10500) / 175;
  } else {
    // m = 1/7000
    // c = 1.5
    // An area of 16000px^2 ~= baseline timing of 0.0005ms.
    complexity = (area + 50000) / 625;
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawDRRect(
    const SkRRect& outer, const SkRRect& inner, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // There are roughly four classes here:
  // a) Filled style with AA enabled.
  // b) Filled style with AA disabled.
  // c) Complex RRect type with AA enabled and filled style.
  // d) Everything else.
  //
  // a) and c) scale linearly with the area, b) and d) scale linearly with
  // a single dimension (length). In all cases, the dimensions refer to
  // the outer RRect.
  unsigned int length = (outer.width() + outer.height()) / 2;

  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style() == SkPaint::Style::kFill_Style) {
    unsigned int area = outer.width() * outer.height();
    if (outer.getType() == SkRRect::Type::kComplex_Type) {
      // m = 1/1000
      // c = 1
      complexity = (area + 1000) / 10;
    } else {
      if (IsAntiAliased()) {
        // m = 1/3500
        // c = 1.5
        complexity = (area + 5250) / 35;
      } else {
        // m = 1/30
        // c = 1
        complexity = (300 + (10 * length)) / 3;
      }
    }
  } else {
    // m = 1/60
    // c = 1.75
    complexity = ((10 * length) + 1050) / 6;
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawPath(
    const SkPath& path, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // There is negligible effect on the performance for hairline vs. non-hairline
  // stroke widths.
  //
  // The data for filled styles is currently suspicious, so for now we are going
  // to assign scores based on stroked styles.

  unsigned int line_verb_cost, quad_verb_cost, conic_verb_cost, cubic_verb_cost;

  if (IsAntiAliased()) {
    line_verb_cost = 75;
    quad_verb_cost = 100;
    conic_verb_cost = 160;
    cubic_verb_cost = 210;
  } else {
    line_verb_cost = 67;
    quad_verb_cost = 80;
    conic_verb_cost = 140;
    cubic_verb_cost = 210;
  }

  // There seems to be a fixed cost of around 1ms for calling drawPath.
  unsigned int complexity =
      200000 + CalculatePathComplexity(path, line_verb_cost, quad_verb_cost,
                                       conic_verb_cost, cubic_verb_cost);

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawArc(
    const SkRect& oval_bounds, SkScalar start_degrees, SkScalar sweep_degrees,
    bool use_center, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // Hairline vs non-hairline makes no difference to the performance.
  // Stroked styles without AA scale linearly with the diameter.
  // Stroked styles with AA scale linearly with the area except for small
  // values. Filled styles scale linearly with the area.
  unsigned int diameter = (oval_bounds.width() + oval_bounds.height()) / 2;
  unsigned int area = oval_bounds.width() * oval_bounds.height();

  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style() == clay::GrPaint::Style::kStroke_Style) {
    if (IsAntiAliased()) {
      // m = 1/8500
      // c = 16
      complexity = (area + 136000) * 2 / 765;
    } else {
      // m = 1/60
      // c = 3
      complexity = (diameter + 180) * 10 / 27;
    }
  } else {
    if (IsAntiAliased()) {
      // m = 1/20000
      // c = 20
      complexity = (area + 400000) / 900;
    } else {
      // m = 1/2100
      // c = 8
      complexity = (area + 16800) * 2 / 189;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawPoints(
    PointMode mode, size_t count, const SkPoint points[],
    const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  unsigned int complexity;

  // If AA is off then they all behave similarly, and scale
  // linearly with the point count.
  if (!IsAntiAliased()) {
    // m = 1/16000
    // c = 0.75
    complexity = (count + 12000) * 25 / 2;
  } else {
    if (mode == SkCanvas::kPolygon_PointMode) {
      // m = 1/1250
      // c = 1
      complexity = (count + 1250) * 160;
    } else {
      if (IsHairline() && mode == SkCanvas::kPoints_PointMode) {
        // This is a special case, it triggers an extremely fast path.
        // m = 1/14500
        // c = 0
        complexity = count * 400 / 29;
      } else {
        // m = 1/2200
        // c = 0.75
        complexity = (count + 1650) * 1000 / 11;
      }
    }
  }
  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawVerticesObject(
    const SkVertices* vertices, SkBlendMode mode, const SkPaint& paint) {
  current_paint_ = paint;
  // There is currently no way for us to get the VertexMode from the SkVertices
  // object, but for future reference:
  //
  // TriangleStrip is roughly 25% more expensive than TriangleFan.
  // TriangleFan is roughly 5% more expensive than Triangles.

  // There is currently no way for us to get the vertex count from an SkVertices
  // object, so we have to estimate it from the approximate size.
  //
  // Approximate size returns the sum of the sizes of the positions (SkPoint),
  // texs (SkPoint), colors (SkColor) and indices (uint16_t) arrays multiplied
  // by sizeof(type). As a very, very rough estimate, divide that by 20 to get
  // an idea of the vertex count.
  unsigned int approximate_vertex_count = vertices->approximateSize() / 20;

  // For the baseline, it's hard to identify the trend. It might be O(n^1/2).
  // For now, treat it as linear as an approximation.
  //
  // m = 1/4000
  // c = 1
  unsigned int complexity = (approximate_vertex_count + 4000) * 50;

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawImage2(
    const SkImage* image, SkScalar left, SkScalar top,
    const SkSamplingOptions& sampling, const SkPaint* paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint ? *paint : SkPaint();
  // AA vs non-AA has a cost but it's dwarfed by the overall cost of the
  // drawImage call.
  //
  // The main difference is if the image is backed by a texture already or not
  // If we don't need to upload, then the cost scales linearly with the
  // area of the image. If it needs uploading, the cost scales linearly
  // with the square of the area (!!!).
  SkISize dimensions = image->dimensions();
  unsigned int area = dimensions.width() * dimensions.height();

  // m = 1/17000
  // c = 3
  unsigned int complexity = (area + 51000) * 4 / 170;

  if (!image->isTextureBacked()) {
    // We can't square the area here as we'll overflow, so let's approximate
    // by taking the calculated complexity score and applying a multiplier to
    // it.
    //
    // (complexity * area / 35000) + 1200 gives a reasonable approximation.
    float multiplier = area / 35000.0f;
    complexity = complexity * multiplier + 1200;
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawPicture(
    const SkPicture* picture, const SkMatrix* matrix, const SkPaint* paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint ? *paint : SkPaint();
  MetalHelper helper(
      Ceiling() - CurrentComplexityScore(),
      SkIRect::MakeLTRB(picture->cullRect().left(), picture->cullRect().top(),
                        picture->cullRect().right(),
                        picture->cullRect().bottom()));
  picture->playback(&helper);
  AccumulateComplexity(helper.ComplexityScore());
}

void PictureMetalComplexityCalculator::MetalHelper::onDrawTextBlob(
    const SkTextBlob* text_blob, SkScalar x, SkScalar y, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // DrawTextBlob has a high fixed cost, but if we call it multiple times
  // per frame, that fixed cost is greatly reduced per subsequent call. This
  // is likely because there is batching being done in SkCanvas.

  // Increment draw_text_blob_count_ and calculate the cost at the end.
  draw_text_blob_count_++;
}

void PictureMetalComplexityCalculator::MetalHelper::ImageRect(
    const SkISize& size, bool texture_backed,
    SkCanvas::SrcRectConstraint constraint) {
  if (IsComplex()) {
    return;
  }
  // Two main groups here - texture-backed and non-texture-backed images.
  //
  // Within each group, they all perform within a few % of each other *except*
  // when we have a strict constraint and anti-aliasing enabled.
  unsigned int area = size.width() * size.height();

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  unsigned int complexity;
  if (texture_backed) {
    // Baseline for texture-backed SkImages.
    // m = 1/23000
    // c = 2.3
    complexity = (area + 52900) * 2 / 115;
    if (current_paint_ != SkPaint() &&
        constraint == SkCanvas::SrcRectConstraint::kStrict_SrcRectConstraint &&
        IsAntiAliased()) {
      // There's about a 30% performance penalty from the baseline.
      complexity *= 1.3f;
    }
  } else {
    if (current_paint_ != SkPaint() &&
        constraint == SkCanvas::SrcRectConstraint::kStrict_SrcRectConstraint &&
        IsAntiAliased()) {
      // m = 1/12200
      // c = 2.75
      complexity = (area + 33550) * 2 / 61;
    } else {
      // m = 1/14500
      // c = 2.5
      complexity = (area + 36250) * 4 / 145;
    }
  }

  AccumulateComplexity(complexity);
}
#else

unsigned int
PictureMetalComplexityCalculator::MetalHelper::BatchedComplexity() {
  // Calculate the impact of saveLayer.
  unsigned int save_layer_complexity;
  if (save_layer_count_ == 0) {
    save_layer_complexity = 0;
  } else {
    // saveLayer seems to have two trends; if the count is < 200,
    // then the individual cost of a saveLayer is higher than if
    // the count is > 200.
    //
    // However, the trend is strange and we should gather more data to
    // get a better idea of how to represent the trend. That being said, it's
    // very unlikely we'll ever hit a DisplayList with 200+ saveLayer calls
    // in it, so we will calculate based on the more reasonably anticipated
    // range of less than 200, with the trend line more weighted towards the
    // lower end of that range (as the data itself doesn't present as a straight
    // line). Further, we will easily hit our cache thresholds with such a
    // large number of saveLayer calls.
    //
    // m = 1/2
    // c = 1
    save_layer_complexity = (save_layer_count_ + 2) * 100000;
  }

  unsigned int draw_text_blob_complexity;
  if (draw_text_blob_count_ == 0) {
    draw_text_blob_complexity = 0;
  } else {
    // m = 1/240
    // c = 0.75
    draw_text_blob_complexity = (draw_text_blob_count_ + 180) * 2500 / 3;
  }

  return save_layer_complexity + draw_text_blob_complexity;
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawLine(
    float x0, float y0, float x1, float y1, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }
  // The curve here may be log-linear, although it doesn't really match up that
  // well. To avoid costly computations, try and do a best fit of the data onto
  // a linear graph as a very rough first order approximation.

  float non_hairline_penalty = 1.0f;
  float aa_penalty = 1.0f;

  if (!IsHairline(paint)) {
    non_hairline_penalty = 1.15f;
  }
  if (IsAntiAliased(paint)) {
    aa_penalty = 1.4f;
  }

  // Use an approximation for the distance to avoid floating point or
  // sqrt() calls.
  float distance = abs(x0 - x1) + abs(y0 - y1);

  // The baseline complexity is for a hairline stroke with no AA.
  // m = 1/45
  // c = 5
  unsigned int complexity =
      ((distance + 225) * 4 / 9) * non_hairline_penalty * aa_penalty;

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawCircle(
    float cx, float cy, float radius, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  unsigned int complexity;

  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style(paint) == skity::Paint::Style::kFill_Style) {
    // We can ignore pi here.
    unsigned int area = radius * radius;
    // m = 1/1300
    // c = 5
    complexity = (area + 6500) * 2 / 65;

    // Penalty of around 5% when AA is disabled.
    if (!IsAntiAliased(paint)) {
      complexity *= 1.05f;
    }
  } else {
    // Hairline vs non-hairline has no significant performance difference.
    if (IsAntiAliased(paint)) {
      // m = 1/7
      // c = 7
      complexity = (radius + 49) * 40 / 7;
    } else {
      // m = 1/16
      // c = 8
      complexity = (radius + 128) * 5 / 2;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawOval(
    skity::Rect const& oval, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }
  // DrawOval scales very roughly linearly with the bounding box width/height
  // (not area) for stroked styles without AA.
  //
  // Filled styles and stroked styles with AA scale linearly with the bounding
  // box area.
  unsigned int area = oval.Width() * oval.Height();

  unsigned int complexity;

  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style(paint) == skity::Paint::Style::kFill_Style) {
    // With filled styles, there is no significant AA penalty.
    // m = 1/16000
    // c = 0
    complexity = area / 80;
  } else {
    if (IsAntiAliased(paint)) {
      // m = 1/7500
      // c = 0
      complexity = area * 2 / 75;
    } else {
      // Take the average of the width and height.
      unsigned int length = (oval.Width() + oval.Height()) / 2;

      // m = 1/80
      // c = 0
      complexity = length * 5 / 2;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawRect(
    skity::Rect const& rect, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  unsigned int complexity;

  // If stroked, cost scales linearly with the rectangle width/height.
  // If filled, it scales with the area.
  //
  // Hairline stroke vs non hairline has no real penalty at smaller lengths,
  // but it increases at larger lengths. There isn't enough data to get a good
  // idea of the penalty at lengths > 1000px.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style(paint) == skity::Paint::Style::kFill_Style) {
    // No real difference for AA with filled styles.
    unsigned int area = rect.Width() * rect.Height();

    // m = 1/9000
    // c = 0
    complexity = area / 225;
  } else {
    // Take the average of the width and height.
    unsigned int length = (rect.Width() + rect.Height()) / 2;

    // There is a penalty for AA being *disabled*.
    if (IsAntiAliased(paint)) {
      // m = 1/65
      // c = 0
      complexity = length * 8 / 13;
    } else {
      // m = 1/35
      // c = 0
      complexity = length * 8 / 7;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawRRect(
    skity::RRect const& rrect, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }
  // RRects scale linearly with the area of the bounding rect.
  unsigned int area = rrect.Width() * rrect.Height();

  // Drawing RRects is split into two performance tiers; an expensive
  // one and a less expensive one. Both scale linearly with area.
  //
  // Expensive: All filled style, symmetric w/AA.
  bool expensive = (Style(paint) == skity::Paint::Style::kFill_Style) ||
                   ((rrect.GetType() == skity::RRect::Type::kSimple) &&
                    IsAntiAliased(paint));

  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  if (expensive) {
    // m = 1/25000
    // c = 2
    // An area of 7000px^2 ~= baseline timing of 0.0005ms.
    complexity = (area + 10500) / 175;
  } else {
    // m = 1/7000
    // c = 1.5
    // An area of 16000px^2 ~= baseline timing of 0.0005ms.
    complexity = (area + 50000) / 625;
  }

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawPath(
    skity::Path const& path, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }
  // There is negligible effect on the performance for hairline vs. non-hairline
  // stroke widths.
  //
  // The data for filled styles is currently suspicious, so for now we are going
  // to assign scores based on stroked styles.

  unsigned int line_verb_cost, quad_verb_cost, conic_verb_cost, cubic_verb_cost;

  if (IsAntiAliased(paint)) {
    line_verb_cost = 75;
    quad_verb_cost = 100;
    conic_verb_cost = 160;
    cubic_verb_cost = 210;
  } else {
    line_verb_cost = 67;
    quad_verb_cost = 80;
    conic_verb_cost = 140;
    cubic_verb_cost = 210;
  }

  // There seems to be a fixed cost of around 1ms for calling drawPath.
  unsigned int complexity =
      200000 + CalculatePathComplexity(path, line_verb_cost, quad_verb_cost,
                                       conic_verb_cost, cubic_verb_cost);

  AccumulateComplexity(complexity);
}

void PictureMetalComplexityCalculator::MetalHelper::OnSaveLayer(
    const skity::Rect& bounds, const skity::Paint& paint) {
  if (IsComplex()) {
    return;
  }

  save_layer_count_++;
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawBlob(
    const skity::TextBlob* blob, float x, float y, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  // DrawTextBlob has a high fixed cost, but if we call it multiple times
  // per frame, that fixed cost is greatly reduced per subsequent call. This
  // is likely because there is batching being done in SkCanvas.

  // Increment draw_text_blob_count_ and calculate the cost at the end.
  draw_text_blob_count_++;
}

void PictureMetalComplexityCalculator::MetalHelper::OnDrawImageRect(
    std::shared_ptr<skity::Image> image, const skity::Rect& src,
    const skity::Rect& dst, const skity::SamplingOptions& sampling,
    skity::Paint const* paint) {
  if (IsComplex()) {
    return;
  }
  // Two main groups here - texture-backed and non-texture-backed images.
  //
  // Within each group, they all perform within a few % of each other *except*
  // when we have a strict constraint and anti-aliasing enabled.
  size_t width = image->Width();
  size_t height = image->Height();
  unsigned int area = width * height;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  unsigned int complexity;
  bool texture_backed = image->IsTextureBackend();
  if (texture_backed) {
    // Baseline for texture-backed SkImages.
    // m = 1/23000
    // c = 2.3
    complexity = (area + 52900) * 2 / 115;
    if (paint && IsAntiAliased(*paint)) {
      // There's about a 30% performance penalty from the baseline.
      complexity *= 1.3f;
    }
  } else {
    if (paint && IsAntiAliased(*paint)) {
      // m = 1/12200
      // c = 2.75
      complexity = (area + 33550) * 2 / 61;
    } else {
      // m = 1/14500
      // c = 2.5
      complexity = (area + 36250) * 4 / 145;
    }
  }

  AccumulateComplexity(complexity);
}

#endif  // ENABLE_SKITY

}  // namespace clay
