// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/embedded_views.h"

#ifndef ENABLE_SKITY
#include "clay/gfx/skity_to_skia_utils.h"
#endif

namespace clay {
#ifndef ENABLE_SKITY
SkPictureEmbedderViewSlice::SkPictureEmbedderViewSlice(
    skity::Rect view_bounds) {
  auto rtree_factory = RTreeFactory();
  rtree_ = rtree_factory.getInstance();
  recorder_ = std::make_unique<SkPictureRecorder>();
  recorder_->beginRecording(clay::ConvertSkityRectToSkRect(view_bounds),
                            &rtree_factory);
}
SkCanvas* SkPictureEmbedderViewSlice::canvas() {
  return recorder_->getRecordingCanvas();
}

void SkPictureEmbedderViewSlice::end_recording() {
  picture_ = recorder_->finishRecordingAsPicture();
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
  canvas->drawPicture(picture_);
}
#else
SkityPictureEmbedderViewSlice::SkityPictureEmbedderViewSlice(
    skity::Rect view_bounds) {
  recorder_ = std::make_unique<skity::PictureRecorder>();
  recorder_->BeginRecording();
}

skity::Canvas* SkityPictureEmbedderViewSlice::canvas() {
  return recorder_->GetRecordingCanvas();
}

void SkityPictureEmbedderViewSlice::end_recording() {
  picture_ = recorder_->FinishRecording();
}

std::list<skity::Rect>
SkityPictureEmbedderViewSlice::searchNonOverlappingDrawnRects(
    const skity::Rect& query) const {
  return {query};
}

void SkityPictureEmbedderViewSlice::render_into(skity::Canvas* canvas) {
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
