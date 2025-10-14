// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_GL_H_
#define CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_GL_H_

#include <limits>
#include <memory>

#include "clay/flow/flow_rendering_backend.h"
#include "clay/flow/layers/picture_complexity.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class PictureGLComplexityCalculator : public PictureComplexityCalculator {
 public:
  static PictureGLComplexityCalculator* GetInstance();

#ifndef ENABLE_SKITY
  unsigned int Compute(SkPicture* picture) override {
    GLHelper helper(ceiling_, SkIRect::MakeLTRB(picture->cullRect().left(),
                                                picture->cullRect().top(),
                                                picture->cullRect().right(),
                                                picture->cullRect().bottom()));
    picture->playback(&helper);
    return helper.ComplexityScore();
  }
#else
  unsigned int Compute(skity::DisplayList* display_list) override {
    GLHelper helper(ceiling_);
    display_list->Draw(&helper);
    return helper.ComplexityScore();
  }
#endif

  bool ShouldBeCached(unsigned int complexity_score) override {
    // Set cache threshold at 1ms
    return complexity_score > 200000u;
  }

  void SetComplexityCeiling(unsigned int ceiling) override {
    ceiling_ = ceiling;
  }

 private:
#ifndef ENABLE_SKITY
  class GLHelper : public ComplexityCalculatorHelper {
   public:
    GLHelper(unsigned int ceiling, SkIRect bounds)
        : ComplexityCalculatorHelper(ceiling, bounds),
          save_layer_count_(0),
          draw_text_blob_count_(0) {}

    void onDrawRect(const SkRect&, const SkPaint&) override;
    void onDrawOval(const SkRect&, const SkPaint&) override;
    void onDrawRRect(const SkRRect&, const SkPaint&) override;
    void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) override;
    void onDrawPath(const SkPath&, const SkPaint&) override;
    void onDrawArc(const SkRect&, SkScalar, SkScalar, bool,
                   const SkPaint&) override;
    void onDrawPoints(PointMode, size_t, const SkPoint[],
                      const SkPaint&) override;
    void onDrawVerticesObject(const SkVertices*, SkBlendMode,
                              const SkPaint&) override;
    void onDrawImage2(const SkImage*, SkScalar, SkScalar,
                      const SkSamplingOptions&, const SkPaint*) override;
    void onDrawPicture(const SkPicture*, const SkMatrix*,
                       const SkPaint*) override;
    void onDrawTextBlob(const SkTextBlob*, SkScalar, SkScalar,
                        const SkPaint&) override;

   protected:
    void ImageRect(const SkISize& size, bool texture_backed,
                   SkCanvas::SrcRectConstraint constraint) override;

    unsigned int BatchedComplexity() override;

   private:
    unsigned int save_layer_count_;
    unsigned int draw_text_blob_count_;
  };
#else
  class GLHelper : public ComplexityCalculatorHelperSkity {
   public:
    explicit GLHelper(unsigned int ceiling)
        : ComplexityCalculatorHelperSkity(ceiling),
          save_layer_count_(0),
          draw_text_blob_count_(0) {}
    void OnDrawLine(float x0, float y0, float x1, float y1,
                    skity::Paint const& paint) override;
    void OnDrawCircle(float cx, float cy, float radius,
                      skity::Paint const& paint) override;
    void OnDrawOval(skity::Rect const& oval,
                    skity::Paint const& paint) override;
    void OnDrawRect(skity::Rect const& rect,
                    skity::Paint const& paint) override;
    void OnDrawRRect(skity::RRect const& rrect,
                     skity::Paint const& paint) override;
    void OnDrawPath(skity::Path const& path,
                    skity::Paint const& paint) override;
    void OnSaveLayer(const skity::Rect& bounds,
                     const skity::Paint& paint) override;
    void OnDrawBlob(const skity::TextBlob* blob, float x, float y,
                    skity::Paint const& paint) override;
    void OnDrawImageRect(std::shared_ptr<skity::Image> image,
                         const skity::Rect& src, const skity::Rect& dst,
                         const skity::SamplingOptions& sampling,
                         skity::Paint const* paint) override;

    unsigned int BatchedComplexity() override;

   private:
    unsigned int save_layer_count_;
    unsigned int draw_text_blob_count_;
  };
#endif

  PictureGLComplexityCalculator()
      : ceiling_(std::numeric_limits<unsigned int>::max()) {}
  static PictureGLComplexityCalculator* instance_;

  unsigned int ceiling_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_GL_H_
