// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_EMBEDDED_VIEWS_H_
#define CLAY_FLOW_EMBEDDED_VIEWS_H_

#include <list>
#include <memory>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/raster_thread_merger.h"
#include "clay/flow/surface_frame.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/image_filter.h"

namespace clay {

enum MutatorType {
  kClipRect,
  kClipRRect,
  kClipPath,
  kTransform,
  kOpacity,
  kBackdropFilter
};

// Represents an image filter mutation.
//
// Should be used for image_filter_layer and backdrop_filter_layer.
// TODO(cyanglaz): Refactor this into a ImageFilterMutator class.
// https://github.com/flutter/flutter/issues/108470
class ImageFilterMutation {
 public:
  ImageFilterMutation(std::shared_ptr<const clay::ImageFilter> filter,
                      const skity::Rect& filter_rect)
      : filter_(filter), filter_rect_(filter_rect) {}

  const clay::ImageFilter& GetFilter() const { return *filter_; }
  const skity::Rect& GetFilterRect() const { return filter_rect_; }

  bool operator==(const ImageFilterMutation& other) const {
    return *filter_ == *other.filter_ && filter_rect_ == other.filter_rect_;
  }

  bool operator!=(const ImageFilterMutation& other) const {
    return !operator==(other);
  }

 private:
  std::shared_ptr<const clay::ImageFilter> filter_;
  const skity::Rect filter_rect_;
};

// Stores mutation information like clipping or kTransform.
//
// The `type` indicates the type of the mutation: kClipRect, kTransform and etc.
// Each `type` is paired with an object that supports the mutation. For example,
// if the `type` is kClipRect, `rect()` is used the represent the rect to be
// clipped. One mutation object must only contain one type of mutation.
class Mutator {
 public:
  Mutator(const Mutator& other) {
    type_ = other.type_;
    switch (other.type_) {
      case kClipRect:
        rect_ = other.rect_;
        break;
      case kClipRRect:
        rrect_ = other.rrect_;
        break;
      case kClipPath:
        path_ = new clay::GrPath(*other.path_);
        break;
      case kTransform:
        matrix_ = other.matrix_;
        break;
      case kOpacity:
        alpha_ = other.alpha_;
        break;
      case kBackdropFilter:
        filter_mutation_ = other.filter_mutation_;
        break;
      default:
        break;
    }
  }

  explicit Mutator(const skity::Rect& rect) : type_(kClipRect), rect_(rect) {}
  explicit Mutator(const skity::RRect& rrect)
      : type_(kClipRRect), rrect_(rrect) {}
  explicit Mutator(const clay::GrPath& path)
      : type_(kClipPath), path_(new clay::GrPath(path)) {}
  explicit Mutator(const skity::Matrix& matrix)
      : type_(kTransform), matrix_(matrix) {}
  explicit Mutator(const int& alpha) : type_(kOpacity), alpha_(alpha) {}
  explicit Mutator(std::shared_ptr<const clay::ImageFilter> filter,
                   const skity::Rect& filter_rect)
      : type_(kBackdropFilter),
        filter_mutation_(
            std::make_shared<ImageFilterMutation>(filter, filter_rect)) {}

  const MutatorType& GetType() const { return type_; }
  const skity::Rect& GetRect() const { return rect_; }
  const skity::RRect& GetRRect() const { return rrect_; }
  const clay::GrPath& GetPath() const { return *path_; }
  const skity::Matrix& GetMatrix() const { return matrix_; }
  const ImageFilterMutation& GetFilterMutation() const {
    return *filter_mutation_;
  }
  const int& GetAlpha() const { return alpha_; }
  float GetAlphaFloat() const { return (alpha_ / 255.0); }

  bool operator==(const Mutator& other) const {
    if (type_ != other.type_) {
      return false;
    }
    switch (type_) {
      case kClipRect:
        return rect_ == other.rect_;
      case kClipRRect:
        return rrect_ == other.rrect_;
      case kClipPath:
        return *path_ == *other.path_;
      case kTransform:
        return matrix_ == other.matrix_;
      case kOpacity:
        return alpha_ == other.alpha_;
      case kBackdropFilter:
        return *filter_mutation_ == *other.filter_mutation_;
    }

    return false;
  }

  bool operator!=(const Mutator& other) const { return !operator==(other); }

  bool IsClipType() {
    return type_ == kClipRect || type_ == kClipRRect || type_ == kClipPath;
  }

  ~Mutator() {
    if (type_ == kClipPath) {
      delete path_;
    }
  }

 private:
  MutatorType type_;

  // TODO(cyanglaz): Remove union.
  //  https://github.com/flutter/flutter/issues/108470
  union {
    skity::Rect rect_;
    skity::RRect rrect_;
    skity::Matrix matrix_;
    clay::GrPath* path_;
    int alpha_;
  };

  std::shared_ptr<ImageFilterMutation> filter_mutation_;
};  // Mutator

// A stack of mutators that can be applied to an embedded platform view.
//
// The stack may include mutators like transforms and clips, each mutator
// applies to all the mutators that are below it in the stack and to the
// embedded view.
//
// For example consider the following stack: [T1, T2, T3], where T1 is the top
// of the stack and T3 is the bottom of the stack. Applying this mutators stack
// to a platform view P1 will result in T1(T2(T3(P1))).
class MutatorsStack {
 public:
  MutatorsStack() = default;

  void PushClipRect(const skity::Rect& rect);
  void PushClipRRect(const skity::RRect& rrect);
  void PushClipPath(const clay::GrPath& path);
  void PushTransform(const skity::Matrix& matrix);
  void PushOpacity(const int& alpha);
  void PushBackdropFilter(
      const std::shared_ptr<const clay::ImageFilter>& filter,
      const skity::Rect& filter_rect);

  // Removes the `Mutator` on the top of the stack
  // and destroys it.
  void Pop();

  void PopTo(size_t stack_count);

  // Returns a reverse iterator pointing to the top of the stack, which is the
  // mutator that is furtherest from the leaf node.
  const std::vector<std::shared_ptr<Mutator>>::const_reverse_iterator Top()
      const;
  // Returns a reverse iterator pointing to the bottom of the stack, which is
  // the mutator that is closest from the leaf node.
  const std::vector<std::shared_ptr<Mutator>>::const_reverse_iterator Bottom()
      const;

  // Returns an iterator pointing to the beginning of the mutator vector, which
  // is the mutator that is furtherest from the leaf node.
  const std::vector<std::shared_ptr<Mutator>>::const_iterator Begin() const;

  // Returns an iterator pointing to the end of the mutator vector, which is the
  // mutator that is closest from the leaf node.
  const std::vector<std::shared_ptr<Mutator>>::const_iterator End() const;

  bool is_empty() const { return vector_.empty(); }
  size_t stack_count() const { return vector_.size(); }

  bool operator==(const MutatorsStack& other) const {
    if (vector_.size() != other.vector_.size()) {
      return false;
    }
    for (size_t i = 0; i < vector_.size(); i++) {
      if (*vector_[i] != *other.vector_[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator==(const std::vector<Mutator>& other) const {
    if (vector_.size() != other.size()) {
      return false;
    }
    for (size_t i = 0; i < vector_.size(); i++) {
      if (*vector_[i] != other[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const MutatorsStack& other) const {
    return !operator==(other);
  }

  bool operator!=(const std::vector<Mutator>& other) const {
    return !operator==(other);
  }

 private:
  std::vector<std::shared_ptr<Mutator>> vector_;
};  // MutatorsStack

class EmbeddedViewParams {
 public:
  EmbeddedViewParams() = default;

  EmbeddedViewParams(skity::Matrix matrix, skity::Vec2 size_points,
                     MutatorsStack mutators_stack)
      : matrix_(matrix),
        size_points_(size_points),
        mutators_stack_(mutators_stack) {
    clay::GrPath path;
    skity::Rect starting_rect =
        skity::Rect::MakeWH(size_points.x, size_points.y);
    PATH_ADD_RECT(path, starting_rect);
    PATH_TRANSFORM(path, matrix);
    RECT_ASSIGN(final_bounding_rect_, PATH_GET_BOUNDS(path));
  }

  // The transformation Matrix corresponding to the sum of all the
  // transformations in the platform view's mutator stack.
  const skity::Matrix& transformMatrix() const { return matrix_; }
  // The original size of the platform view before any mutation matrix is
  // applied.
  const skity::Vec2& sizePoints() const { return size_points_; }
  // The mutators stack contains the detailed step by step mutations for this
  // platform view.
  const MutatorsStack& mutatorsStack() const { return mutators_stack_; }
  // The bounding rect of the platform view after applying all the mutations.
  //
  // Clippings are ignored.
  const skity::Rect& finalBoundingRect() const { return final_bounding_rect_; }

  // Pushes the stored DlImageFilter object to the mutators stack.
  void PushImageFilter(std::shared_ptr<const clay::ImageFilter> filter,
                       const skity::Rect& filter_rect) {
    mutators_stack_.PushBackdropFilter(filter, filter_rect);
  }

  bool operator==(const EmbeddedViewParams& other) const {
    return size_points_ == other.size_points_ &&
           mutators_stack_ == other.mutators_stack_ &&
           final_bounding_rect_ == other.final_bounding_rect_ &&
           matrix_ == other.matrix_;
  }

 private:
  skity::Matrix matrix_;
  skity::Vec2 size_points_;
  MutatorsStack mutators_stack_;
  skity::Rect final_bounding_rect_;
};

// The |PlatformViewLayer| calls |CompositeEmbeddedView| in its |Paint|
// method to replace the leaf_nodes_canvas and leaf_nodes_builder in its
// |PaintContext| for subsequent layers in the frame to render into.
// The builder value will only be supplied if the associated ScopedFrame
// is being rendered to DisplayLists. The |EmbedderPaintContext| struct
// allows the method to return both values.
struct EmbedderPaintContext {
  clay::GrCanvas* canvas;
};

// The |EmbedderViewSlice| represents the details of recording all of
// the layer tree rendering operations that appear between before, after
// and between the embedded views. The Slice may be recorded into an
// SkPicture.
class EmbedderViewSlice {
 public:
  virtual ~EmbedderViewSlice() = default;
  virtual clay::GrCanvas* canvas() = 0;
  virtual void render_into(clay::GrCanvas* canvas) = 0;
  virtual void end_recording() = 0;
  virtual std::list<skity::Rect> searchNonOverlappingDrawnRects(
      const skity::Rect& query) const = 0;
};

#ifndef ENABLE_SKITY
class SkPictureEmbedderViewSlice : public EmbedderViewSlice {
 public:
  explicit SkPictureEmbedderViewSlice(skity::Rect view_bounds);
  ~SkPictureEmbedderViewSlice() override = default;

  SkCanvas* canvas() override;
  void end_recording() override;
  std::list<skity::Rect> searchNonOverlappingDrawnRects(
      const skity::Rect& query) const override;
  void render_into(SkCanvas* canvas) override;

 private:
  std::unique_ptr<SkPictureRecorder> recorder_;
  sk_sp<RTree> rtree_;
  sk_sp<SkPicture> picture_;
};
#else
class SkityPictureEmbedderViewSlice : public EmbedderViewSlice {
 public:
  explicit SkityPictureEmbedderViewSlice(skity::Rect view_bounds);
  ~SkityPictureEmbedderViewSlice() override = default;

  skity::Canvas* canvas() override;
  void end_recording() override;
  std::list<skity::Rect> searchNonOverlappingDrawnRects(
      const skity::Rect& query) const override;
  void render_into(skity::Canvas* canvas) override;

 private:
  std::unique_ptr<skity::PictureRecorder> recorder_;
  std::unique_ptr<skity::DisplayList> picture_;
};
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_FLOW_EMBEDDED_VIEWS_H_
