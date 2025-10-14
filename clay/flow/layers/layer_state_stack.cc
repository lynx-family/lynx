// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/layer_state_stack.h"

#include <algorithm>

#include "clay/flow/layers/layer.h"
#include "clay/flow/matrix_clip_tracker.h"
#include "clay/flow/paint_utils.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

// ==============================================================
// Delegate subclasses
// ==============================================================

// The DummyDelegate class implements most Delegate methods as NOP
// but throws errors if the caller starts executing query methods
// that require an active delegate to be tracking. It is specifically
// designed to be immutable, lightweight, and a singleton so that it
// can be substituted into the delegate slot in a LayerStateStack
// quickly and cheaply when no externally supplied delegates are present.
class DummyDelegate : public LayerStateStack::Delegate {
 public:
  static const std::shared_ptr<DummyDelegate> kInstance;

  void decommission() override {}

  skity::Rect local_cull_rect() const override {
    error();
    return {};
  }
  skity::Rect device_cull_rect() const override {
    error();
    return {};
  }
  skity::Matrix matrix_4x4() const override {
    error();
    return {};
  }
  bool content_culled(const skity::Rect& content_bounds) const override {
    error();
    return true;
  }

  void save() override {}
  void saveLayer(const skity::Rect& bounds,
                 LayerStateStack::RenderingAttributes& attributes,
                 clay::BlendMode blend,
                 const clay::ImageFilter* backdrop) override {}
  void restore() override {}

  void translate(float tx, float ty) override {}
  void transform(const skity::Matrix& m44) override {}
  void integralTransform() override {}

  void clipRect(const skity::Rect& rect, clay::GrClipOp op,
                bool is_aa) override {}
  void clipRRect(const skity::RRect& rrect, clay::GrClipOp op,
                 bool is_aa) override {}
  void clipPath(const clay::GrPath& path, clay::GrClipOp op,
                bool is_aa) override {}

 private:
  static void error() {
    FML_DCHECK(false) << "LayerStateStack state queried without a delegate";
  }
};
const std::shared_ptr<DummyDelegate> DummyDelegate::kInstance =
    std::make_shared<DummyDelegate>();

#ifndef ENABLE_SKITY
class SkCanvasDelegate : public LayerStateStack::Delegate {
 public:
  explicit SkCanvasDelegate(SkCanvas* canvas)
      : canvas_(canvas), initial_save_level_(canvas->getSaveCount()) {}

  void decommission() override { canvas_->restoreToCount(initial_save_level_); }

  SkCanvas* canvas() const override { return canvas_; }

  skity::Rect local_cull_rect() const override {
    return clay::ConvertSkRectToSkityRect(canvas_->getLocalClipBounds());
  }
  skity::Rect device_cull_rect() const override {
    return clay::ConvertSkIRectToSkityRect(canvas_->getDeviceClipBounds());
  }
  skity::Matrix matrix_4x4() const override {
    return clay::ConvertSkM44ToMatrix(canvas_->getLocalToDevice());
  }
  bool content_culled(const skity::Rect& content_bounds) const override {
    return canvas_->quickReject(clay::ConvertSkityRectToSkRect(content_bounds));
  }

  void save() override { canvas_->save(); }
  void saveLayer(const skity::Rect& bounds,
                 LayerStateStack::RenderingAttributes& attributes,
                 clay::BlendMode blend_mode,
                 const clay::ImageFilter* backdrop) override {
    TRACE_EVENT("clay", "Canvas::saveLayer");
    SkPaint paint;
    sk_sp<SkImageFilter> backdrop_filter =
        backdrop ? backdrop->gr_object() : nullptr;
    SkRect tmp = clay::ConvertSkityRectToSkRect(bounds);
    canvas_->saveLayer(SkCanvas::SaveLayerRec(
        &tmp, attributes.fill(paint, blend_mode), backdrop_filter.get(), 0));
  }
  void restore() override { canvas_->restore(); }

  void translate(float tx, float ty) override { canvas_->translate(tx, ty); }
  void transform(const skity::Matrix& m44) override {
    canvas_->concat(clay::ConvertSkityMatrixToSkM44(m44));
  }
  void integralTransform() override {
    skity::Matrix matrix = RasterCacheUtil::GetIntegralTransCTM(matrix_4x4());
    canvas_->setMatrix(clay::ConvertSkityMatrixToSkM44(matrix));
  }

  void clipRect(const skity::Rect& rect, SkClipOp op, bool is_aa) override {
    canvas_->clipRect(clay::ConvertSkityRectToSkRect(rect), op, is_aa);
  }
  void clipRRect(const skity::RRect& rrect, SkClipOp op, bool is_aa) override {
    canvas_->clipRRect(clay::ConvertSkityRRectToSkia(rrect), op, is_aa);
  }
  void clipPath(const SkPath& path, SkClipOp op, bool is_aa) override {
    canvas_->clipPath(path, op, is_aa);
  }

 private:
  SkCanvas* canvas_;
  const int initial_save_level_;
};
#else
class SkityCanvasDelegate : public LayerStateStack::Delegate {
 public:
  explicit SkityCanvasDelegate(skity::Canvas* canvas)
      : canvas_(canvas), initial_save_level_(canvas->GetSaveCount()) {}

  void decommission() override { canvas_->RestoreToCount(initial_save_level_); }

  skity::Canvas* canvas() const override { return canvas_; }

  skity::Rect local_cull_rect() const override {
    auto rect = canvas_->GetLocalClipBounds();
    return skity::Rect::MakeXYWH(rect.X(), rect.Y(), rect.Width(),
                                 rect.Height());
  }
  skity::Rect device_cull_rect() const override {
    auto rect = canvas_->GetGlobalClipBounds();
    return skity::Rect::MakeXYWH(rect.X(), rect.Y(), rect.Width(),
                                 rect.Height());
  }
  skity::Matrix matrix_4x4() const override {
    return canvas_->GetTotalMatrix();
  }
  bool content_culled(const skity::Rect& content_bounds) const override {
    return canvas_->QuickReject(content_bounds);
  }

  void save() override { canvas_->Save(); }
  void saveLayer(const skity::Rect& bounds,
                 LayerStateStack::RenderingAttributes& attributes,
                 clay::BlendMode blend_mode,
                 const clay::ImageFilter* backdrop) override {
    TRACE_EVENT("flutter", "Canvas::saveLayer");
    skity::Paint paint;
    attributes.fill(paint, blend_mode);
    canvas_->SaveLayer(bounds, paint);
  }
  void restore() override { canvas_->Restore(); }

  void translate(float tx, float ty) override { canvas_->Translate(tx, ty); }
  void transform(const skity::Matrix& m44) override { canvas_->Concat(m44); }
  void integralTransform() override {
    skity::Matrix matrix = RasterCacheUtil::GetIntegralTransCTM(matrix_4x4());
    canvas_->SetMatrix(matrix);
  }

  void clipRect(const skity::Rect& rect, clay::GrClipOp op,
                bool is_aa) override {
    canvas_->ClipRect(rect, static_cast<skity::Canvas::ClipOp>(op));
  }
  void clipRRect(const skity::RRect& rrect, clay::GrClipOp op,
                 bool is_aa) override {
    auto skity_rrect = rrect;
    skity::Path path;
    path.AddRRect(skity_rrect);
    canvas_->ClipPath(path, op);
  }

  void clipPath(const skity::Path& path, clay::GrClipOp op,
                bool is_aa) override {
    canvas_->ClipPath(path, op);
  }

 private:
  skity::Canvas* canvas_;
  const int initial_save_level_;
};
#endif  // ENABLE_SKITY

class PrerollDelegate : public LayerStateStack::Delegate {
 public:
  PrerollDelegate(const skity::Rect& cull_rect, const skity::Matrix& matrix)
      : tracker_(cull_rect, matrix) {}

  void decommission() override {}

  skity::Matrix matrix_4x4() const override { return tracker_.matrix_4x4(); }
  skity::Rect local_cull_rect() const override {
    return tracker_.local_cull_rect();
  }
  skity::Rect device_cull_rect() const override {
    return tracker_.device_cull_rect();
  }
  bool content_culled(const skity::Rect& content_bounds) const override {
    return tracker_.content_culled(content_bounds);
  }

  void save() override { tracker_.save(); }
  void saveLayer(const skity::Rect& bounds,
                 LayerStateStack::RenderingAttributes& attributes,
                 clay::BlendMode blend,
                 const clay::ImageFilter* backdrop) override {
    tracker_.save();
  }
  void restore() override { tracker_.restore(); }

  void translate(float tx, float ty) override { tracker_.translate(tx, ty); }
  void transform(const skity::Matrix& m44) override { tracker_.transform(m44); }
  void integralTransform() override {
    tracker_.setTransform(
        RasterCacheUtil::GetIntegralTransCTM(tracker_.matrix_4x4()));
  }

  void clipRect(const skity::Rect& rect, clay::GrClipOp op,
                bool is_aa) override {
    tracker_.clipRect(rect, op, is_aa);
  }
  void clipRRect(const skity::RRect& rrect, clay::GrClipOp op,
                 bool is_aa) override {
    tracker_.clipRRect(rrect, op, is_aa);
  }
  void clipPath(const clay::GrPath& path, clay::GrClipOp op,
                bool is_aa) override {
    tracker_.clipPath(path, op, is_aa);
  }

 private:
  MatrixClipTracker tracker_;
};

// ==============================================================
// StateEntry subclasses
// ==============================================================

class SaveEntry : public LayerStateStack::StateEntry {
 public:
  SaveEntry() = default;

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->save();
  }
  void restore(LayerStateStack* stack) const override {
    stack->delegate_->restore();
  }

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(SaveEntry);
};

class SaveLayerEntry : public LayerStateStack::StateEntry {
 public:
  SaveLayerEntry(const skity::Rect& bounds, clay::BlendMode blend_mode,
                 const LayerStateStack::RenderingAttributes& prev)
      : bounds_(bounds), blend_mode_(blend_mode), old_attributes_(prev) {}

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->saveLayer(bounds_, stack->outstanding_, blend_mode_,
                                nullptr);
    stack->outstanding_ = {};
  }
  void restore(LayerStateStack* stack) const override {
    if (stack->checkerboard_func_) {
      (*stack->checkerboard_func_)(stack->canvas_delegate(), bounds_);
    }
    stack->delegate_->restore();
    stack->outstanding_ = old_attributes_;
  }

 protected:
  const skity::Rect bounds_;
  const clay::BlendMode blend_mode_;
  const LayerStateStack::RenderingAttributes old_attributes_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(SaveLayerEntry);
};

class OpacityEntry : public LayerStateStack::StateEntry {
 public:
  OpacityEntry(const skity::Rect& bounds, float opacity,
               const LayerStateStack::RenderingAttributes& prev)
      : bounds_(bounds),
        opacity_(opacity),
        old_opacity_(prev.opacity),
        old_bounds_(prev.save_layer_bounds) {}

  void apply(LayerStateStack* stack) const override {
    stack->outstanding_.save_layer_bounds = bounds_;
    stack->outstanding_.opacity *= opacity_;
  }
  void restore(LayerStateStack* stack) const override {
    stack->outstanding_.save_layer_bounds = old_bounds_;
    stack->outstanding_.opacity = old_opacity_;
  }
  void update_mutators(MutatorsStack* mutators_stack) const override {
    mutators_stack->PushOpacity(clay::Color::toAlpha(opacity_));
  }

 private:
  const skity::Rect bounds_;
  const float opacity_;
  const float old_opacity_;
  const skity::Rect old_bounds_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(OpacityEntry);
};

class ImageFilterEntry : public LayerStateStack::StateEntry {
 public:
  ImageFilterEntry(const skity::Rect& bounds,
                   const std::shared_ptr<const clay::ImageFilter>& filter,
                   const LayerStateStack::RenderingAttributes& prev)
      : bounds_(bounds),
        filter_(filter),
        old_filter_(prev.image_filter),
        old_bounds_(prev.save_layer_bounds) {}
  ~ImageFilterEntry() override = default;

  void apply(LayerStateStack* stack) const override {
    stack->outstanding_.save_layer_bounds = bounds_;
    stack->outstanding_.image_filter = filter_;
  }
  void restore(LayerStateStack* stack) const override {
    stack->outstanding_.save_layer_bounds = old_bounds_;
    stack->outstanding_.image_filter = old_filter_;
  }

  // There is no ImageFilter mutator currently
  // void update_mutators(MutatorsStack* mutators_stack) const override;

 private:
  const skity::Rect bounds_;
  const std::shared_ptr<const clay::ImageFilter> filter_;
  const std::shared_ptr<const clay::ImageFilter> old_filter_;
  const skity::Rect old_bounds_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ImageFilterEntry);
};

class ColorFilterEntry : public LayerStateStack::StateEntry {
 public:
  ColorFilterEntry(const skity::Rect& bounds,
                   const std::shared_ptr<const clay::ColorFilter>& filter,
                   const LayerStateStack::RenderingAttributes& prev)
      : bounds_(bounds),
        filter_(filter),
        old_filter_(prev.color_filter),
        old_bounds_(prev.save_layer_bounds) {}
  ~ColorFilterEntry() override = default;

  void apply(LayerStateStack* stack) const override {
    stack->outstanding_.save_layer_bounds = bounds_;
    stack->outstanding_.color_filter = filter_;
  }
  void restore(LayerStateStack* stack) const override {
    stack->outstanding_.save_layer_bounds = old_bounds_;
    stack->outstanding_.color_filter = old_filter_;
  }

  // There is no ColorFilter mutator currently
  // void update_mutators(MutatorsStack* mutators_stack) const override;

 private:
  const skity::Rect bounds_;
  const std::shared_ptr<const clay::ColorFilter> filter_;
  const std::shared_ptr<const clay::ColorFilter> old_filter_;
  const skity::Rect old_bounds_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ColorFilterEntry);
};

class BackdropFilterEntry : public SaveLayerEntry {
 public:
  BackdropFilterEntry(const skity::Rect& bounds,
                      const std::shared_ptr<const clay::ImageFilter>& filter,
                      clay::BlendMode blend_mode,
                      const LayerStateStack::RenderingAttributes& prev)
      : SaveLayerEntry(bounds, blend_mode, prev), filter_(filter) {}
  ~BackdropFilterEntry() override = default;

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->saveLayer(bounds_, stack->outstanding_, blend_mode_,
                                filter_.get());
    stack->outstanding_ = {};
  }

  void reapply(LayerStateStack* stack) const override {
    // On the reapply for subsequent overlay layers, we do not
    // want to reapply the backdrop filter, but we do need to
    // do a saveLayer to encapsulate the contents and match the
    // restore that will be forthcoming. Note that this is not
    // perfect if the BlendMode is not associative as we will be
    // compositing multiple parts of the content in batches.
    // Luckily the most common SrcOver is associative.
    SaveLayerEntry::apply(stack);
  }

 private:
  const std::shared_ptr<const clay::ImageFilter> filter_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(BackdropFilterEntry);
};

class TranslateEntry : public LayerStateStack::StateEntry {
 public:
  TranslateEntry(float tx, float ty) : tx_(tx), ty_(ty) {}

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->translate(tx_, ty_);
  }
  void update_mutators(MutatorsStack* mutators_stack) const override {
    mutators_stack->PushTransform(skity::Matrix::Translate(tx_, ty_));
  }

 private:
  const float tx_;
  const float ty_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(TranslateEntry);
};

class TransformM44Entry : public LayerStateStack::StateEntry {
 public:
  explicit TransformM44Entry(const skity::Matrix& m44) : m44_(m44) {}

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->transform(m44_);
  }
  void update_mutators(MutatorsStack* mutators_stack) const override {
    mutators_stack->PushTransform(m44_);
  }

 private:
  const skity::Matrix m44_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(TransformM44Entry);
};

class IntegralTransformEntry : public LayerStateStack::StateEntry {
 public:
  IntegralTransformEntry() = default;

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->integralTransform();
  }

 private:
  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(IntegralTransformEntry);
};

class ClipRectEntry : public LayerStateStack::StateEntry {
 public:
  ClipRectEntry(const skity::Rect& clip_rect, bool is_aa)
      : clip_rect_(clip_rect), is_aa_(is_aa) {}

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->clipRect(clip_rect_, clay::GrClipOp::kIntersect, is_aa_);
  }
  void update_mutators(MutatorsStack* mutators_stack) const override {
    mutators_stack->PushClipRect(clip_rect_);
  }

 private:
  const skity::Rect clip_rect_;
  const bool is_aa_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ClipRectEntry);
};

class ClipRRectEntry : public LayerStateStack::StateEntry {
 public:
  ClipRRectEntry(const skity::RRect& clip_rrect, bool is_aa)
      : clip_rrect_(clip_rrect), is_aa_(is_aa) {}

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->clipRRect(clip_rrect_, clay::GrClipOp::kIntersect,
                                is_aa_);
  }
  void update_mutators(MutatorsStack* mutators_stack) const override {
    mutators_stack->PushClipRRect(clip_rrect_);
  }

 private:
  const skity::RRect clip_rrect_;
  const bool is_aa_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ClipRRectEntry);
};

class ClipPathEntry : public LayerStateStack::StateEntry {
 public:
  ClipPathEntry(const clay::GrPath& clip_path, bool is_aa)
      : clip_path_(clip_path), is_aa_(is_aa) {}

  ~ClipPathEntry() override = default;

  void apply(LayerStateStack* stack) const override {
    stack->delegate_->clipPath(clip_path_, clay::GrClipOp::kIntersect, is_aa_);
  }
  void update_mutators(MutatorsStack* mutators_stack) const override {
    mutators_stack->PushClipPath(clip_path_);
  }

 private:
  clay::GrPath clip_path_;
  const bool is_aa_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(ClipPathEntry);
};

// ==============================================================
// RenderingAttributes methods
// ==============================================================
clay::GrPaint* LayerStateStack::RenderingAttributes::fill(
    clay::GrPaint& paint, clay::BlendMode mode) const {
  clay::GrPaint* ret = nullptr;
  if (opacity < 1.f) {
    PAINT_SET_ALPHAF(paint, std::max(opacity, 0.0f));
    ret = &paint;
  } else {
    PAINT_SET_ALPHAF(paint, 1.f);
  }
  if (color_filter) {
    PAINT_SET_COLOR_FILTER(paint, color_filter->gr_object());
    ret = &paint;
  } else {
    PAINT_SET_COLOR_FILTER(paint, nullptr);
  }
  if (image_filter) {
    PAINT_SET_IMAGE_FILTER(paint, image_filter->gr_object());
    ret = &paint;
  } else {
    PAINT_SET_IMAGE_FILTER(paint, nullptr);
  }
  PAINT_SET_BLEND_MODE(paint, mode);
  if (mode != clay::BlendMode::kSrcOver) {
    ret = &paint;
  }
  return ret;
}

clay::Paint* LayerStateStack::RenderingAttributes::fill(
    clay::Paint& paint, clay::BlendMode mode) const {
  clay::Paint* ret = nullptr;
  if (opacity < 1.f) {
    paint.setOpacity(std::max(opacity, 0.0f));
    ret = &paint;
  } else {
    paint.setOpacity(1.f);
  }
  paint.setColorFilter(color_filter);
  if (color_filter) {
    ret = &paint;
  }
  paint.setImageFilter(image_filter);
  if (image_filter) {
    ret = &paint;
  }
  paint.setBlendMode(mode);
  if (mode != clay::BlendMode::kSrcOver) {
    ret = &paint;
  }
  return ret;
}

// ==============================================================
// MutatorContext methods
// ==============================================================

using MutatorContext = LayerStateStack::MutatorContext;

void MutatorContext::saveLayer(const skity::Rect& bounds) {
  layer_state_stack_->save_layer(bounds);
}

void MutatorContext::applyOpacity(const skity::Rect& bounds, float opacity) {
  if (opacity < 1.f) {
    layer_state_stack_->push_opacity(bounds, opacity);
  }
}

void MutatorContext::applyImageFilter(
    const skity::Rect& bounds,
    const std::shared_ptr<const clay::ImageFilter>& filter) {
  if (filter) {
    layer_state_stack_->push_image_filter(bounds, filter);
  }
}

void MutatorContext::applyColorFilter(
    const skity::Rect& bounds,
    const std::shared_ptr<const clay::ColorFilter>& filter) {
  if (filter) {
    layer_state_stack_->push_color_filter(bounds, filter);
  }
}

void MutatorContext::applyBackdropFilter(
    const skity::Rect& bounds,
    const std::shared_ptr<const clay::ImageFilter>& filter,
    clay::BlendMode blend_mode) {
  layer_state_stack_->push_backdrop(bounds, filter, blend_mode);
}

void MutatorContext::translate(float tx, float ty) {
  if (!(tx == 0 && ty == 0)) {
    layer_state_stack_->maybe_save_layer_for_transform(save_needed_);
    save_needed_ = false;
    layer_state_stack_->push_translate(tx, ty);
  }
}

void MutatorContext::transform(const skity::Matrix& m44) {
  if (m44.OnlyTranslate()) {
    translate(m44.GetTranslateX(), m44.GetTranslateY());
  } else if (!m44.IsIdentity()) {
    layer_state_stack_->maybe_save_layer_for_transform(save_needed_);
    save_needed_ = false;
    layer_state_stack_->push_transform(m44);
  }
}

void MutatorContext::integralTransform() {
  layer_state_stack_->maybe_save_layer_for_transform(save_needed_);
  save_needed_ = false;
  layer_state_stack_->push_integral_transform();
}

void MutatorContext::clipRect(const skity::Rect& rect, bool is_aa) {
  layer_state_stack_->maybe_save_layer_for_clip(save_needed_);
  save_needed_ = false;
  layer_state_stack_->push_clip_rect(rect, is_aa);
}

void MutatorContext::clipRRect(const skity::RRect& rrect, bool is_aa) {
  layer_state_stack_->maybe_save_layer_for_clip(save_needed_);
  save_needed_ = false;
  layer_state_stack_->push_clip_rrect(rrect, is_aa);
}

void MutatorContext::clipPath(const clay::GrPath& path, bool is_aa) {
  layer_state_stack_->maybe_save_layer_for_clip(save_needed_);
  save_needed_ = false;
  layer_state_stack_->push_clip_path(path, is_aa);
}

// ==============================================================
// LayerStateStack methods
// ==============================================================

LayerStateStack::LayerStateStack() : delegate_(DummyDelegate::kInstance) {}

void LayerStateStack::clear_delegate() {
  delegate_->decommission();
  delegate_ = DummyDelegate::kInstance;
}

void LayerStateStack::set_delegate(clay::GrCanvas* canvas) {
  if (delegate_) {
    if (canvas == delegate_->canvas()) {
      return;
    }
    clear_delegate();
  }
  if (canvas) {
#ifndef ENABLE_SKITY
    delegate_ = std::make_shared<SkCanvasDelegate>(canvas);
#else
    delegate_ = std::make_shared<SkityCanvasDelegate>(canvas);
#endif  // ENABLE_SKITY
    reapply_all();
  }
}

void LayerStateStack::set_preroll_delegate(const skity::Rect& cull_rect) {
  set_preroll_delegate(cull_rect, skity::Matrix());
}
void LayerStateStack::set_preroll_delegate(const skity::Matrix& matrix) {
  set_preroll_delegate(kGiantRect, matrix);
}
void LayerStateStack::set_preroll_delegate(const skity::Rect& cull_rect,
                                           const skity::Matrix& matrix) {
  delegate_ = std::make_shared<PrerollDelegate>(cull_rect, matrix);
  reapply_all();
}

void LayerStateStack::reapply_all() {
  // We use a local RenderingAttributes instance so that it can track the
  // necessary state changes independently as they occur in the stack.
  // Reusing |outstanding_| would wreak havoc on the current state of
  // the stack. When we are finished, though, the local attributes
  // contents should match the current outstanding_ values;
  RenderingAttributes attributes = outstanding_;
  outstanding_ = {};
  for (auto& state : state_stack_) {
    state->reapply(this);
  }
  FML_DCHECK(attributes == outstanding_);
}

void LayerStateStack::fill(MutatorsStack* mutators) {
  for (auto& state : state_stack_) {
    state->update_mutators(mutators);
  }
}

void LayerStateStack::restore_to_count(size_t restore_count) {
  while (state_stack_.size() > restore_count) {
    state_stack_.back()->restore(this);
    state_stack_.pop_back();
  }
}

void LayerStateStack::push_opacity(const skity::Rect& bounds, float opacity) {
  maybe_save_layer(opacity);
  state_stack_.emplace_back(
      std::make_unique<OpacityEntry>(bounds, opacity, outstanding_));
  apply_last_entry();
}

void LayerStateStack::push_color_filter(
    const skity::Rect& bounds,
    const std::shared_ptr<const clay::ColorFilter>& filter) {
  maybe_save_layer(filter);
  state_stack_.emplace_back(
      std::make_unique<ColorFilterEntry>(bounds, filter, outstanding_));
  apply_last_entry();
}

void LayerStateStack::push_image_filter(
    const skity::Rect& bounds,
    const std::shared_ptr<const clay::ImageFilter>& filter) {
  maybe_save_layer(filter);
  state_stack_.emplace_back(
      std::make_unique<ImageFilterEntry>(bounds, filter, outstanding_));
  apply_last_entry();
}

void LayerStateStack::push_backdrop(
    const skity::Rect& bounds,
    const std::shared_ptr<const clay::ImageFilter>& filter,
    clay::BlendMode blend_mode) {
  state_stack_.emplace_back(std::make_unique<BackdropFilterEntry>(
      bounds, filter, blend_mode, outstanding_));
  apply_last_entry();
}

void LayerStateStack::push_translate(float tx, float ty) {
  state_stack_.emplace_back(std::make_unique<TranslateEntry>(tx, ty));
  apply_last_entry();
}

void LayerStateStack::push_transform(const skity::Matrix& m44) {
  state_stack_.emplace_back(std::make_unique<TransformM44Entry>(m44));
  apply_last_entry();
}

void LayerStateStack::push_integral_transform() {
  state_stack_.emplace_back(std::make_unique<IntegralTransformEntry>());
  apply_last_entry();
}

void LayerStateStack::push_clip_rect(const skity::Rect& rect, bool is_aa) {
  state_stack_.emplace_back(std::make_unique<ClipRectEntry>(rect, is_aa));
  apply_last_entry();
}

void LayerStateStack::push_clip_rrect(const skity::RRect& rrect, bool is_aa) {
  state_stack_.emplace_back(std::make_unique<ClipRRectEntry>(rrect, is_aa));
  apply_last_entry();
}

void LayerStateStack::push_clip_path(const clay::GrPath& path, bool is_aa) {
  state_stack_.emplace_back(std::make_unique<ClipPathEntry>(path, is_aa));
  apply_last_entry();
}

bool LayerStateStack::needs_save_layer(int flags) const {
  if (outstanding_.opacity < 1.f &&
      (flags & LayerStateStack::kCallerCanApplyOpacity) == 0) {
    return true;
  }
  if (outstanding_.image_filter &&
      (flags & LayerStateStack::kCallerCanApplyImageFilter) == 0) {
    return true;
  }
  if (outstanding_.color_filter &&
      (flags & LayerStateStack::kCallerCanApplyColorFilter) == 0) {
    return true;
  }
  return false;
}

void LayerStateStack::do_save() {
  state_stack_.emplace_back(std::make_unique<SaveEntry>());
  apply_last_entry();
}

void LayerStateStack::save_layer(const skity::Rect& bounds) {
  state_stack_.emplace_back(std::make_unique<SaveLayerEntry>(
      bounds, clay::BlendMode::kSrcOver, outstanding_));
  apply_last_entry();
}

void LayerStateStack::maybe_save_layer_for_transform(bool save_needed) {
  // Alpha and ColorFilter don't care about transform
  if (outstanding_.image_filter) {
    save_layer(outstanding_.save_layer_bounds);
  } else if (save_needed) {
    do_save();
  }
}

void LayerStateStack::maybe_save_layer_for_clip(bool save_needed) {
  // Alpha and ColorFilter don't care about clipping
  // - Alpha of clipped content == clip of alpha content
  // - Color-filtering of clipped content == clip of color-filtered content
  if (outstanding_.image_filter) {
    save_layer(outstanding_.save_layer_bounds);
  } else if (save_needed) {
    do_save();
  }
}

void LayerStateStack::maybe_save_layer(int apply_flags) {
  if (needs_save_layer(apply_flags)) {
    save_layer(outstanding_.save_layer_bounds);
  }
}

void LayerStateStack::maybe_save_layer(float opacity) {
  if (outstanding_.image_filter) {
    save_layer(outstanding_.save_layer_bounds);
  }
}

void LayerStateStack::maybe_save_layer(
    const std::shared_ptr<const clay::ColorFilter>& filter) {
  if (outstanding_.color_filter || outstanding_.image_filter ||
      (outstanding_.opacity < 1.f && !filter->can_commute_with_opacity())) {
    // TBD: compose the 2 color filters together.
    save_layer(outstanding_.save_layer_bounds);
  }
}

void LayerStateStack::maybe_save_layer(
    const std::shared_ptr<const clay::ImageFilter>& filter) {
  if (outstanding_.image_filter) {
    // TBD: compose the 2 image filters together.
    save_layer(outstanding_.save_layer_bounds);
  }
}

}  // namespace clay
