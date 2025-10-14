// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_complexity_gl.h"

#include <algorithm>
#include <cmath>

// The numbers and weightings used in this file stem from taking the
// data from the DisplayListBenchmarks suite run on an Pixel 4 and
// applying very rough analysis on them to identify the approximate
// trends.
//
// See the comments in picture_complexity_helper.h for details on the
// process and rationale behind coming up with these numbers.

namespace clay {

PictureGLComplexityCalculator* PictureGLComplexityCalculator::instance_ =
    nullptr;

PictureGLComplexityCalculator* PictureGLComplexityCalculator::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new PictureGLComplexityCalculator();
  }
  return instance_;
}

#ifndef ENABLE_SKITY
unsigned int PictureGLComplexityCalculator::GLHelper::BatchedComplexity() {
  // Calculate the impact of saveLayer.
  unsigned int save_layer_complexity;
  if (save_layer_count_ == 0) {
    save_layer_complexity = 0;
  } else {
    // m = 1/5
    // c = 10
    save_layer_complexity = (save_layer_count_ + 50) * 40000;
  }

  unsigned int draw_text_blob_complexity;
  if (draw_text_blob_count_ == 0) {
    draw_text_blob_complexity = 0;
  } else {
    // m = 1/240
    // c = 0.25
    draw_text_blob_complexity = (draw_text_blob_count_ + 60) * 2500 / 3;
  }

  return save_layer_complexity + draw_text_blob_complexity;
}

void PictureGLComplexityCalculator::GLHelper::onDrawRect(const SkRect& rect,
                                                         const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  unsigned int complexity;

  // If stroked, cost scales linearly with the rectangle width/height.
  // If filled, it scales with the area.
  //
  // Hairline stroke vs non hairline has no significant penalty.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style() == SkPaint::Style::kFill_Style) {
    // No real difference for AA with filled styles
    unsigned int area = rect.width() * rect.height();

    // m = 1/3500
    // c = 0
    complexity = area * 2 / 175;
  } else {
    // Take the average of the width and height.
    unsigned int length = (rect.width() + rect.height()) / 2;

    if (IsAntiAliased()) {
      // m = 1/30
      // c = 0
      complexity = length * 4 / 3;
    } else {
      // If AA is disabled, the data shows that at larger sizes the overall
      // cost comes down, peaking at around 1000px. As we don't anticipate
      // rasterising rects with AA disabled to be all that frequent, just treat
      // it as a straight line that peaks at 1000px, beyond which it stays
      // constant. The rationale here is that it makes more sense to
      // overestimate than to start decreasing the cost as the length goes up.
      //
      // This should be a reasonable approximation as it doesn't decrease by
      // much from 1000px to 2000px.
      //
      // m = 1/20
      // c = 0
      complexity = std::min(length, 1000u) * 2;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawOval(const SkRect& bounds,
                                                         const SkPaint& paint) {
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
    // m = 1/6000
    // c = 0
    complexity = area / 30;
  } else {
    if (IsAntiAliased()) {
      // m = 1/4000
      // c = 0
      complexity = area / 20;
    } else {
      // Take the average of the width and height.
      unsigned int length = (bounds.width() + bounds.height()) / 2;

      // m = 1/75
      // c = 0
      complexity = length * 8 / 3;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawRRect(
    const SkRRect& rrect, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }

  current_paint_ = paint;

  // Drawing RRects is split into three performance tiers:
  //
  // 1) All stroked styles without AA *except* simple/symmetric RRects.
  // 2) All filled styles and symmetric stroked styles w/AA.
  // 3) Remaining stroked styles with AA.
  //
  // 1) and 3) scale linearly with length, 2) scales with area.

  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  if (Style() == SkPaint::Style::kFill_Style ||
      ((rrect.getType() == SkRRect::Type::kSimple_Type) && IsAntiAliased())) {
    unsigned int area = rrect.width() * rrect.height();
    // m = 1/3200
    // c = 0.5
    complexity = (area + 1600) / 80;
  } else {
    // Take the average of the width and height.
    unsigned int length = (rrect.width() + rrect.height()) / 2;

    // There is some difference between hairline and non-hairline performance
    // but the spread is relatively inconsistent and it's pretty much a wash.
    if (IsAntiAliased()) {
      // m = 1/25
      // c = 1
      complexity = (length + 25) * 8 / 5;
    } else {
      // m = 1/50
      // c = 0.75
      complexity = ((length * 2) + 75) * 2 / 5;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawDRRect(
    const SkRRect& outer, const SkRRect& inner, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // There are roughly four classes here:
  // a) Filled style.
  // b) Complex RRect type with AA enabled and filled style.
  // c) Stroked style with AA enabled.
  // d) Stroked style with AA disabled.
  //
  // a) and b) scale linearly with the area, c) and d) scale linearly with
  // a single dimension (length). In all cases, the dimensions refer to
  // the outer RRect.

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
      // m = 1/500
      // c = 0.5
      complexity = (area + 250) / 5;
    } else {
      // m = 1/1600
      // c = 2
      complexity = (area + 3200) / 16;
    }
  } else {
    unsigned int length = (outer.width() + outer.height()) / 2;
    if (IsAntiAliased()) {
      // m = 1/15
      // c = 1
      complexity = (length + 15) * 20 / 3;
    } else {
      // m = 1/27
      // c = 0.5
      complexity = ((length * 2) + 27) * 50 / 27;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawPath(const SkPath& path,
                                                         const SkPaint& paint) {
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
  unsigned int complexity;

  if (IsAntiAliased()) {
    // There seems to be a fixed cost of around 1ms for calling drawPath with
    // AA.
    complexity = 200000;

    line_verb_cost = 235;
    quad_verb_cost = 365;
    conic_verb_cost = 365;
    cubic_verb_cost = 725;
  } else {
    // There seems to be a fixed cost of around 0.25ms for calling drawPath.
    // without AA
    complexity = 50000;

    line_verb_cost = 135;
    quad_verb_cost = 150;
    conic_verb_cost = 200;
    cubic_verb_cost = 235;
  }

  complexity += CalculatePathComplexity(path, line_verb_cost, quad_verb_cost,
                                        conic_verb_cost, cubic_verb_cost);

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawArc(
    const SkRect& oval_bounds, SkScalar start_degrees, SkScalar sweep_degrees,
    bool use_center, const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  // Hairline vs non-hairline makes no difference to the performance.
  // Stroked styles without AA scale linearly with the log of the diameter.
  // Stroked styles with AA scale linearly with the area.
  // Filled styles scale linearly with the area.
  unsigned int area = oval_bounds.width() * oval_bounds.height();
  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style() == SkPaint::Style::kStroke_Style) {
    if (IsAntiAliased()) {
      // m = 1/3800
      // c = 12
      complexity = (area + 45600) / 171;
    } else {
      unsigned int diameter = (oval_bounds.width() + oval_bounds.height()) / 2;
      // m = 15
      // c = -100
      // This should never go negative though, so use std::max to ensure
      // c is never larger than 15*log_diameter.
      //
      // Pre-multiply by 15 here so we get a little bit more precision.
      unsigned int log_diameter = 15 * log(diameter);
      complexity = (log_diameter - std::max(log_diameter, 100u)) * 200 / 9;
    }
  } else {
    if (IsAntiAliased()) {
      // m = 1/1000
      // c = 10
      complexity = (area + 10000) / 45;
    } else {
      // m = 1/6500
      // c = 12
      complexity = (area + 52000) * 2 / 585;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawPoints(
    PointMode mode, size_t count, const SkPoint points[],
    const SkPaint& paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint;
  unsigned int complexity;

  if (IsAntiAliased()) {
    if (mode == SkCanvas::kPoints_PointMode) {
      if (IsHairline()) {
        // This is a special case, it triggers an extremely fast path.
        // m = 1/4500
        // c = 0
        complexity = count * 400 / 9;
      } else {
        // m = 1/500
        // c = 0
        complexity = count * 400;
      }
    } else if (mode == SkCanvas::kLines_PointMode) {
      if (IsHairline()) {
        // m = 1/750
        // c = 0
        complexity = count * 800 / 3;
      } else {
        // m = 1/500
        // c = 0
        complexity = count * 400;
      }
    } else {
      if (IsHairline()) {
        // m = 1/350
        // c = 0
        complexity = count * 4000 / 7;
      } else {
        // m = 1/250
        // c = 0
        complexity = count * 800;
      }
    }
  } else {
    if (mode == SkCanvas::kPoints_PointMode) {
      // Hairline vs non hairline makes no difference for points without AA.
      // m = 1/18000
      // c = 0.25
      complexity = (count + 4500) * 100 / 9;
    } else if (mode == SkCanvas::kLines_PointMode) {
      if (IsHairline()) {
        // m = 1/8500
        // c = 0.25
        complexity = (count + 2125) * 400 / 17;
      } else {
        // m = 1/9000
        // c = 0.25
        complexity = (count + 2250) * 200 / 9;
      }
    } else {
      // Polygon only really diverges for hairline vs non hairline at large
      // point counts, and only by a few %.
      // m = 1/7500
      // c = 0.25
      complexity = (count + 1875) * 80 / 3;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawVerticesObject(
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

  // For the baseline, it's hard to identify the trend. It might be O(n^1/2)
  // For now, treat it as linear as an approximation.
  //
  // m = 1/1600
  // c = 1
  unsigned int complexity = (approximate_vertex_count + 1600) * 250 / 2;

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawImage2(
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
  // length of the image. If it needs uploading, the cost scales linearly
  // with the square of the area (!!!).
  SkISize dimensions = image->dimensions();
  unsigned int length = (dimensions.width() + dimensions.height()) / 2;
  unsigned int area = dimensions.width() * dimensions.height();

  // m = 1/13
  // c = 0
  unsigned int complexity = length * 400 / 13;

  if (!image->isTextureBacked()) {
    // We can't square the area here as we'll overflow, so let's approximate
    // by taking the calculated complexity score and applying a multiplier to
    // it.
    //
    // (complexity * area / 60000) + 4000 gives a reasonable approximation with
    // AA (complexity * area / 19000) gives a reasonable approximation without
    // AA.
    float multiplier;
    if (IsAntiAliased()) {
      multiplier = area / 60000.0f;
      complexity = complexity * multiplier + 4000;
    } else {
      multiplier = area / 19000.0f;
      complexity = complexity * multiplier;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::onDrawPicture(
    const SkPicture* picture, const SkMatrix* matrix, const SkPaint* paint) {
  if (IsComplex()) {
    return;
  }
  current_paint_ = paint ? *paint : SkPaint();
  GLHelper helper(
      Ceiling() - CurrentComplexityScore(),
      SkIRect::MakeLTRB(picture->cullRect().left(), picture->cullRect().top(),
                        picture->cullRect().right(),
                        picture->cullRect().bottom()));
  picture->playback(&helper);
  AccumulateComplexity(helper.ComplexityScore());
}

void PictureGLComplexityCalculator::GLHelper::onDrawTextBlob(
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

void PictureGLComplexityCalculator::GLHelper::ImageRect(
    const SkISize& size, bool texture_backed,
    SkCanvas::SrcRectConstraint constraint) {
  if (IsComplex()) {
    return;
  }
  // Two main groups here - texture-backed and non-texture-backed images.
  //
  // Within each group, they all perform within a few % of each other *except*
  // when we have a strict constraint and anti-aliasing enabled.

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  unsigned int complexity;
  if (!texture_backed ||
      (texture_backed && current_paint_ != SkPaint() &&
       constraint == SkCanvas::SrcRectConstraint::kStrict_SrcRectConstraint &&
       IsAntiAliased())) {
    unsigned int area = size.width() * size.height();
    // m = 1/4000
    // c = 5
    complexity = (area + 20000) / 10;
  } else {
    unsigned int length = (size.width() + size.height()) / 2;
    // There's a little bit of spread here but the numbers are pretty large
    // anyway.
    //
    // m = 1/22
    // c = 0
    complexity = length * 200 / 11;
  }

  AccumulateComplexity(complexity);
}
#else
unsigned int PictureGLComplexityCalculator::GLHelper::BatchedComplexity() {
  // Calculate the impact of saveLayer.
  unsigned int save_layer_complexity;
  if (save_layer_count_ == 0) {
    save_layer_complexity = 0;
  } else {
    // m = 1/5
    // c = 10
    save_layer_complexity = (save_layer_count_ + 50) * 40000;
  }

  unsigned int draw_text_blob_complexity;
  if (draw_text_blob_count_ == 0) {
    draw_text_blob_complexity = 0;
  } else {
    // m = 1/240
    // c = 0.25
    draw_text_blob_complexity = (draw_text_blob_count_ + 60) * 2500 / 3;
  }

  return save_layer_complexity + draw_text_blob_complexity;
}

void PictureGLComplexityCalculator::GLHelper::OnDrawLine(
    float x0, float y0, float x1, float y1, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  // There is a relatively high fixed overhead cost for drawLine on OpenGL.
  // Further, there is a strange bump where the cost of drawing a line of
  // length ~500px is actually more costly than drawing a line of length
  // ~1000px. The calculations here will be for a linear graph that
  // approximate the overall trend.

  float non_hairline_penalty = 1.0f;
  unsigned int aa_penalty = 1;

  // The non-hairline penalty is insignificant when AA is on.
  if (!IsHairline(paint) && !IsAntiAliased(paint)) {
    non_hairline_penalty = 1.15f;
  }
  if (IsAntiAliased(paint)) {
    aa_penalty = 2;
  }

  // Use an approximation for the distance to avoid floating point or
  // sqrt() calls.
  float distance = abs(x0 - x1) + abs(y0 - y1);

  // The baseline complexity is for a hairline stroke with no AA.
  // m = 1/40
  // c = 13
  unsigned int complexity =
      ((distance + 520) / 2) * non_hairline_penalty * aa_penalty;

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::OnDrawCircle(
    float cx, float cy, float radius, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  unsigned int complexity;

  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style(paint) == skity::Paint::Style::kFill_Style) {
    // We can ignore pi here
    unsigned int area = radius * radius;
    // m = 1/525
    // c = 50
    complexity = (area + 26250) * 8 / 105;

    // Penalty of around 8% when AA is disabled.
    if (!IsAntiAliased(paint)) {
      complexity *= 1.08f;
    }
  } else {
    // Hairline vs non-hairline has no significant performance difference.
    if (IsAntiAliased(paint)) {
      // m = 1/3
      // c = 10
      complexity = (radius + 30) * 40 / 3;
    } else {
      // m = 1/10
      // c = 20
      complexity = (radius + 200) * 4;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::OnDrawOval(
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
    // m = 1/6000
    // c = 0
    complexity = area / 30;
  } else {
    if (IsAntiAliased(paint)) {
      // m = 1/4000
      // c = 0
      complexity = area / 20;
    } else {
      // Take the average of the width and height.
      unsigned int length = (oval.Width() + oval.Height()) / 2;

      // m = 1/75
      // c = 0
      complexity = length * 8 / 3;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::OnDrawRect(
    skity::Rect const& rect, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  unsigned int complexity;

  // If stroked, cost scales linearly with the rectangle width/height.
  // If filled, it scales with the area.
  //
  // Hairline stroke vs non hairline has no significant penalty.
  //
  // There is also a kStrokeAndFill_Style that Skia exposes, but we do not
  // currently use it anywhere in Flutter.
  if (Style(paint) == skity::Paint::Style::kFill_Style) {
    // No real difference for AA with filled styles
    unsigned int area = rect.Width() * rect.Height();

    // m = 1/3500
    // c = 0
    complexity = area * 2 / 175;
  } else {
    // Take the average of the width and height.
    unsigned int length = (rect.Width() + rect.Height()) / 2;

    if (IsAntiAliased(paint)) {
      // m = 1/30
      // c = 0
      complexity = length * 4 / 3;
    } else {
      // If AA is disabled, the data shows that at larger sizes the overall
      // cost comes down, peaking at around 1000px. As we don't anticipate
      // rasterising rects with AA disabled to be all that frequent, just treat
      // it as a straight line that peaks at 1000px, beyond which it stays
      // constant. The rationale here is that it makes more sense to
      // overestimate than to start decreasing the cost as the length goes up.
      //
      // This should be a reasonable approximation as it doesn't decrease by
      // much from 1000px to 2000px.
      //
      // m = 1/20
      // c = 0
      complexity = std::min(length, 1000u) * 2;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::OnDrawRRect(
    skity::RRect const& rrect, skity::Paint const& paint) {
  if (IsComplex()) {
    return;
  }

  // Drawing RRects is split into three performance tiers:
  //
  // 1) All stroked styles without AA *except* simple/symmetric RRects.
  // 2) All filled styles and symmetric stroked styles w/AA.
  // 3) Remaining stroked styles with AA.
  //
  // 1) and 3) scale linearly with length, 2) scales with area.

  unsigned int complexity;

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  if (Style(paint) == skity::Paint::Style::kFill_Style ||
      ((rrect.GetType() == skity::RRect::Type::kSimple) &&
       IsAntiAliased(paint))) {
    unsigned int area = rrect.Width() * rrect.Height();
    // m = 1/3200
    // c = 0.5
    complexity = (area + 1600) / 80;
  } else {
    // Take the average of the width and height.
    unsigned int length = (rrect.Width() + rrect.Height()) / 2;

    // There is some difference between hairline and non-hairline performance
    // but the spread is relatively inconsistent and it's pretty much a wash.
    if (IsAntiAliased(paint)) {
      // m = 1/25
      // c = 1
      complexity = (length + 25) * 8 / 5;
    } else {
      // m = 1/50
      // c = 0.75
      complexity = ((length * 2) + 75) * 2 / 5;
    }
  }

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::OnDrawPath(
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
  unsigned int complexity;

  if (IsAntiAliased(paint)) {
    // There seems to be a fixed cost of around 1ms for calling drawPath with
    // AA.
    complexity = 200000;

    line_verb_cost = 235;
    quad_verb_cost = 365;
    conic_verb_cost = 365;
    cubic_verb_cost = 725;
  } else {
    // There seems to be a fixed cost of around 0.25ms for calling drawPath.
    // without AA
    complexity = 50000;

    line_verb_cost = 135;
    quad_verb_cost = 150;
    conic_verb_cost = 200;
    cubic_verb_cost = 235;
  }

  complexity += CalculatePathComplexity(path, line_verb_cost, quad_verb_cost,
                                        conic_verb_cost, cubic_verb_cost);

  AccumulateComplexity(complexity);
}

void PictureGLComplexityCalculator::GLHelper::OnSaveLayer(
    const skity::Rect& bounds, const skity::Paint& paint) {
  if (IsComplex()) {
    return;
  }

  save_layer_count_++;
}

void PictureGLComplexityCalculator::GLHelper::OnDrawBlob(
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

void PictureGLComplexityCalculator::GLHelper::OnDrawImageRect(
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

  // These values were worked out by creating a straight line graph (y=mx+c)
  // approximately matching the measured data, normalising the data so that
  // 0.0005ms resulted in a score of 100 then simplifying down the formula.
  unsigned int complexity;
  bool texture_backed = image->IsTextureBackend();
  size_t width = image->Width();
  size_t height = image->Height();
  if (!texture_backed || (paint && texture_backed && IsAntiAliased(*paint))) {
    unsigned int area = width * height;
    // m = 1/4000
    // c = 5
    complexity = (area + 20000) / 10;
  } else {
    unsigned int length = (width + height) / 2;
    // There's a little bit of spread here but the numbers are pretty large
    // anyway.
    //
    // m = 1/22
    // c = 0
    complexity = length * 200 / 11;
  }

  AccumulateComplexity(complexity);
}

#endif  // ENABLE_SKITY

}  // namespace clay
