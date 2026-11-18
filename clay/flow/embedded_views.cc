// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/embedded_views.h"

#include "base/trace/native/trace_event.h"
#ifndef ENABLE_SKITY
#include "clay/gfx/skity_to_skia_utils.h"
#include "third_party/skia/include/utils/SkNoDrawCanvas.h"
#include "third_party/skia/include/utils/SkPaintFilterCanvas.h"
#endif

namespace clay {
#ifndef ENABLE_SKITY
namespace {
// A flattened image is only equivalent to the recording if its paints do not
// read or replace pixels in earlier scene entries. Retain those uncommon
// operations for display playback instead of changing their blend semantics.
class RasterCacheAnalyzer final : public SkPaintFilterCanvas {
 public:
  explicit RasterCacheAnalyzer(SkCanvas* canvas)
      : SkPaintFilterCanvas(canvas) {}
  mutable bool safe = true;

 protected:
  bool onFilter(SkPaint& paint) const override {
    if (paint.asBlendMode() != SkBlendMode::kSrcOver) safe = false;
    return true;
  }
  void onDrawPicture(const SkPicture* picture, const SkMatrix* matrix,
                     const SkPaint* paint) override {
    SkAutoCanvasRestore restore(this, true);
    if (matrix) concat(*matrix);
    if (paint) {
      SkPaint copy = *paint;
      onFilter(copy);
    }
    picture->playback(this);
  }
  SaveLayerStrategy getSaveLayerStrategy(const SaveLayerRec& rec) override {
    if (rec.fBackdrop) safe = false;
    if (rec.fPaint) {
      SkPaint paint = *rec.fPaint;
      onFilter(paint);
    }
    return SkPaintFilterCanvas::getSaveLayerStrategy(rec);
  }
  void onDrawDrawable(SkDrawable*, const SkMatrix*) override { safe = false; }
  bool onDoSaveBehind(const SkRect*) override {
    safe = false;
    return false;
  }
  void onDrawEdgeAAQuad(const SkRect&, const SkPoint[4], QuadAAFlags,
                        const SkColor4f&, SkBlendMode mode) override {
    if (mode != SkBlendMode::kSrcOver) safe = false;
  }
};
}  // namespace

SkPictureEmbedderViewSlice::SkPictureEmbedderViewSlice(
    skity::Rect view_bounds) {
  auto rtree_factory = RTreeFactory();
  rtree_ = rtree_factory.getInstance();
  recorder_ = std::make_unique<SkPictureRecorder>();
  recorder_->beginRecording(clay::ConvertSkityRectToSkRect(view_bounds),
                            &rtree_factory);
}
SkCanvas* SkPictureEmbedderViewSlice::canvas() {
  return recorder_ ? recorder_->getRecordingCanvas() : nullptr;
}

void SkPictureEmbedderViewSlice::end_recording() {
  if (recorder_) {
    picture_ = recorder_->finishRecordingAsPicture();
    recorder_.reset();
  }
}

bool SkPictureEmbedderViewSlice::can_rasterize() const {
  if (!picture_) return false;
  SkIRect bounds;
  picture_->cullRect().roundOut(&bounds);
  SkNoDrawCanvas canvas(bounds.width(), bounds.height());
  canvas.translate(-bounds.left(), -bounds.top());
  RasterCacheAnalyzer analyzer(&canvas);
  picture_->playback(&analyzer);
  return analyzer.safe;
}

std::list<skity::Rect>
SkPictureEmbedderViewSlice::searchNonOverlappingDrawnRects(
    const skity::Rect& query) const {
  std::list<skity::Rect> result;
  auto sk_result = rtree_->searchNonOverlappingDrawnRects(
      clay::ConvertSkityRectToSkRect(query));
  for (auto& rect : sk_result) {
    result.push_back(clay::ConvertSkRectToSkityRect(rect));
  }
  return result;
}

void SkPictureEmbedderViewSlice::render_into(SkCanvas* canvas) {
  TRACE_EVENT("clay", "SkPictureEmbedderViewSlice::render_into");
  canvas->drawPicture(picture_);
}
#else
namespace {
// A balanced saveLayer isolates destination-dependent draws inside the
// recording. Only operations that can affect the incoming destination prevent
// rasterizing a static slice into a transferable image.
class RasterCacheAnalyzer final : public skity::Canvas {
 public:
  explicit RasterCacheAnalyzer(const skity::Rect& bounds) : Canvas(bounds) {}
  bool safe = true;

 protected:
  void Check(const skity::Paint& paint) {
    if (!layers_ && paint.GetBlendMode() != skity::BlendMode::kSrcOver)
      safe = false;
  }
  void OnClipPath(const skity::Path&, ClipOp) override {}
  void OnDrawPath(const skity::Path&, const skity::Paint& paint) override {
    Check(paint);
  }
  void OnDrawPaint(const skity::Paint& paint) override { Check(paint); }
  void OnDrawBlob(const skity::TextBlob*, float, float,
                  const skity::Paint& paint) override {
    Check(paint);
  }
  void OnDrawGlyphs(uint32_t, const skity::GlyphID[], const float[],
                    const float[], const skity::Font&,
                    const skity::Paint& paint) override {
    Check(paint);
  }
  void OnDrawImageRect(std::shared_ptr<skity::Image>, const skity::Rect&,
                       const skity::Rect&, const skity::SamplingOptions&,
                       const skity::Paint* paint) override {
    if (paint) Check(*paint);
  }
  void OnSaveLayer(const skity::Rect&, const skity::Paint& paint) override {
    Check(paint);
    saves_.push_back(true);
    ++layers_;
  }
  void OnSave() override { saves_.push_back(false); }
  void OnRestore() override {
    if (!saves_.empty()) {
      layers_ -= saves_.back();
      saves_.pop_back();
    }
  }
  void OnRestoreToCount(int count) override {
    while (!saves_.empty() && saves_.size() >= static_cast<size_t>(count))
      OnRestore();
  }
  void OnFlush() override {}
  uint32_t OnGetWidth() const override { return 0; }
  uint32_t OnGetHeight() const override { return 0; }

 private:
  size_t layers_ = 0;
  std::vector<bool> saves_;
};
}  // namespace

SkityPictureEmbedderViewSlice::SkityPictureEmbedderViewSlice(
    skity::Rect view_bounds) {
  recorder_ = std::make_unique<skity::PictureRecorder>();
  skity::DisplayListBuildOptions options;
  options.build_rtree = true;
  recorder_->BeginRecording(view_bounds, options);
}

skity::Canvas* SkityPictureEmbedderViewSlice::canvas() {
  return recorder_ ? recorder_->GetRecordingCanvas() : nullptr;
}

void SkityPictureEmbedderViewSlice::end_recording() {
  if (recorder_) {
    picture_ = recorder_->FinishRecording();
    recorder_.reset();
  }
}

bool SkityPictureEmbedderViewSlice::can_rasterize() const {
  if (!picture_) return false;
  RasterCacheAnalyzer analyzer(picture_->GetBounds());
  picture_->Draw(&analyzer);
  return analyzer.safe;
}

std::list<skity::Rect>
SkityPictureEmbedderViewSlice::searchNonOverlappingDrawnRects(
    const skity::Rect& query) const {
  std::list<skity::Rect> result;
  auto skity_result = picture_->SearchNonOverlappingDrawnRects(query);
  result.insert(result.end(), skity_result.begin(), skity_result.end());
  return result;
}

void SkityPictureEmbedderViewSlice::render_into(skity::Canvas* canvas) {
  TRACE_EVENT("clay", "SkityPictureEmbedderViewSlice::render_into");
  picture_->Draw(canvas);
}
#endif  // ENABLE_SKITY

void MutatorsStack::PushClipRect(const skity::Rect& rect) {
  std::shared_ptr<Mutator> element = std::make_shared<Mutator>(rect);
  vector_.push_back(element);
};

void MutatorsStack::PushClipRRect(const skity::RRect& rrect) {
  std::shared_ptr<Mutator> element = std::make_shared<Mutator>(rrect);
  vector_.push_back(element);
};

void MutatorsStack::PushClipPath(const clay::GrPath& path) {
  std::shared_ptr<Mutator> element = std::make_shared<Mutator>(path);
  vector_.push_back(element);
}

void MutatorsStack::PushTransform(const skity::Matrix& matrix) {
  std::shared_ptr<Mutator> element = std::make_shared<Mutator>(matrix);
  vector_.push_back(element);
};

void MutatorsStack::PushOpacity(const int& alpha) {
  std::shared_ptr<Mutator> element = std::make_shared<Mutator>(alpha);
  vector_.push_back(element);
};

void MutatorsStack::PushBackdropFilter(
    const std::shared_ptr<const clay::ImageFilter>& filter,
    const skity::Rect& filter_rect) {
  std::shared_ptr<Mutator> element =
      std::make_shared<Mutator>(filter, filter_rect);
  vector_.push_back(element);
};

void MutatorsStack::Pop() { vector_.pop_back(); };

void MutatorsStack::PopTo(size_t stack_count) {
  while (vector_.size() > stack_count) {
    Pop();
  }
}

const std::vector<std::shared_ptr<Mutator>>::const_reverse_iterator
MutatorsStack::Top() const {
  return vector_.rend();
};

const std::vector<std::shared_ptr<Mutator>>::const_reverse_iterator
MutatorsStack::Bottom() const {
  return vector_.rbegin();
};

const std::vector<std::shared_ptr<Mutator>>::const_iterator
MutatorsStack::Begin() const {
  return vector_.begin();
};

const std::vector<std::shared_ptr<Mutator>>::const_iterator MutatorsStack::End()
    const {
  return vector_.end();
};

}  // namespace clay
