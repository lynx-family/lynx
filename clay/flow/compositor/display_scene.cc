// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor/display_scene.h"

#include <list>
#include <utility>

#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/drawable_image_layer.h"

namespace clay {
namespace {
class DisplaySceneSlice final : public EmbedderViewSlice {
 public:
  DisplaySceneSlice(std::shared_ptr<DisplayScene> scene, int64_t view_id,
                    GrContext* context)
      : scene_(std::move(scene)), view_id_(view_id), context_(context) {}
  GrCanvas* canvas() override { return nullptr; }
  void end_recording() override {}
  void render_into(GrCanvas* canvas) override {
    scene_->Paint(canvas, context_, view_id_);
  }
  std::list<skity::Rect> searchNonOverlappingDrawnRects(
      const skity::Rect& query) const override {
    return scene_->SearchSlice(view_id_, query);
  }

 private:
  std::shared_ptr<DisplayScene> scene_;
  int64_t view_id_;
  GrContext* context_;
};

void RestoreGroup(GrCanvas* canvas, GrContext*, const skity::Matrix&) {
  CANVAS_RESTORE(canvas);
  CANVAS_RESTORE(canvas);
}

bool Visit(const Layer& layer, bool& has_image) {
  if (!layer.SupportsDisplayScene()) return false;
  if (layer.as_drawable_image_layer()) has_image = true;
  if (auto* container = layer.as_container_layer()) {
    for (const auto& child : container->layers()) {
      if (!Visit(*child, has_image)) return false;
    }
  }
  return true;
}
}  // namespace

bool DisplayScene::CanRecord(const Layer& root) {
  bool has_image = false;
  return Visit(root, has_image) && has_image;
}

DisplayScene::DisplayScene(const skity::Rect& bounds,
                           const skity::Matrix& root_transform)
    : bounds_(bounds), root_transform_(root_transform) {
  root_transform.Invert(&inverse_root_);
}
DisplayScene::~DisplayScene() = default;

void DisplayScene::StartRecording(PaintContext& context) {
#ifndef ENABLE_SKITY
  recording_ = std::make_unique<SkPictureEmbedderViewSlice>(bounds_);
#else
  recording_ = std::make_unique<SkityPictureEmbedderViewSlice>(bounds_);
#endif
  context.canvas = recording_->canvas();
  CANVAS_CONCAT(context.canvas, root_transform_);
  context.state_stack.set_delegate(context.canvas);
}

void DisplayScene::FinishRecording(PaintContext& context) {
  context.state_stack.set_delegate(static_cast<GrCanvas*>(nullptr));
  recording_->end_recording();
  auto drawn = recording_->searchNonOverlappingDrawnRects(bounds_);
  skity::Rect bounds = skity::Rect::MakeEmpty();
  for (const auto& rect : drawn) bounds.Join(rect);
  const bool can_rasterize = recording_->can_rasterize();
  // Destructive blends may not contribute a drawn rect to the spatial index.
  // They still have to execute against earlier scene entries.
  if (bounds.IsEmpty() && !can_rasterize) bounds = bounds_;
  // Avoid allocating images for recordings containing only state changes.
  if (!bounds.IsEmpty()) {
    bounds.Outset(1);
    bounds.RoundOut();
    bounds.Intersect(bounds_);
    Entry entry;
    entry.can_rasterize = can_rasterize;
    prepared_ = false;
    entry.recording = std::move(recording_);
    entry.bounds = bounds;
    entry.slice = current_slice_;
    entries_.push_back(std::move(entry));
  }
  recording_.reset();
}

void DisplayScene::Begin(PaintContext& context) {
  StartRecording(context);
  context.state_stack.set_save_layer_handler(
      [this, &context](const skity::Rect* bounds, const GrPaint* paint,
                       const std::shared_ptr<const ImageFilter>& backdrop) {
        RecordGroup(context, bounds, paint, backdrop);
      });
}

void DisplayScene::RecordGroup(
    PaintContext& context, const skity::Rect* bounds, const GrPaint* paint,
    const std::shared_ptr<const ImageFilter>& backdrop) {
  Entry entry;
  entry.slice = current_slice_;
  if (bounds) {
    auto state = context.state_stack.CaptureTransformAndClip();
    auto transform = context.state_stack.transform_4x4();
    auto make_draw = [root = root_transform_, state, bounds = *bounds,
                      paint =
                          *paint](std::shared_ptr<const ImageFilter> filter) {
      return [root, state, bounds, paint, filter](GrCanvas* canvas, GrContext*,
                                                  const skity::Matrix& base) {
        CANVAS_SAVE(canvas);
        CANVAS_CONCAT(canvas, root);
        state(canvas);
#ifndef ENABLE_SKITY
        const auto rect = ConvertSkityRectToSkRect(bounds);
        auto backdrop = filter ? filter->gr_object() : nullptr;
        canvas->saveLayer(
            SkCanvas::SaveLayerRec(&rect, &paint, backdrop.get(), 0));
#else
        // Match the Skity LayerStateStack delegate's saveLayer behavior.
        CANVAS_SAVELAYER(canvas, bounds, paint);
#endif
        CANVAS_RESET_MATRIX(canvas);
        CANVAS_CONCAT(canvas, base);
      };
    };
    entry.draw = make_draw(backdrop);
    // A platform-view boundary resumes the group without sampling the
    // backdrop again, matching LayerStateStack::BackdropFilterEntry::reapply.
    auto mapped = transform.MapRect(*bounds);
    entry.bounds = mapped;
    groups_.push_back({make_draw(nullptr), mapped});
  } else {
    groups_.pop_back();
    entry.draw = RestoreGroup;
  }
  FinishRecording(context);
  entries_.push_back(std::move(entry));
  StartRecording(context);
}

void DisplayScene::RecordImage(const DrawableImageLayer& layer,
                               PaintContext& context) {
  auto draw = layer.CaptureDisplayPaint(context);
  auto bounds =
      context.state_stack.transform_4x4().MapRect(layer.paint_bounds());
  bounds.Intersect(context.state_stack.device_cull_rect());
  FinishRecording(context);
  Entry entry;
  entry.slice = current_slice_;
  entry.bounds = bounds;
  if (!layer.frozen()) entry.update_image_id = layer.image_id();
  entry.draw = [root = root_transform_, draw = std::move(draw)](
                   GrCanvas* canvas, GrContext* context, const skity::Matrix&) {
    GrAutoCanvasRestore restore(canvas, true);
    CANVAS_CONCAT(canvas, root);
    draw(canvas, context);
  };
  entries_.push_back(std::move(entry));
  StartRecording(context);
}

void DisplayScene::RecordGroupInput(
    PaintContext& context, const skity::Rect& bounds,
    const std::function<void(PaintContext&)>& paint) {
  const auto transform = context.state_stack.transform_4x4();
  const auto outer_bounds = bounds_;
  const auto outer_root = root_transform_;
  FinishRecording(context);
  // Filters can sample outside the viewport or an ancestor clip. Preserve
  // their whole input; the display saveLayer clips the filtered output.
  bounds_ = transform.MapRect(bounds);
  bounds_.Outset(1);
  bounds_.RoundOut();
  root_transform_ = skity::Matrix();
  LayerStateStack input_stack;
  PaintContext input = {
      .state_stack = input_stack,
      .canvas = nullptr,
      .gr_context = context.gr_context,
      .sample_count = context.sample_count,
#ifndef ENABLE_SKITY
      .dst_color_space = context.dst_color_space,
#endif
      .compositor_state = context.compositor_state,
      .raster_time = context.raster_time,
      .ui_time = context.ui_time,
      .drawable_image_registry = context.drawable_image_registry,
      .raster_cache = nullptr,
      .frame_device_pixel_ratio = context.frame_device_pixel_ratio,
      .display_scene = this,
      .only_draw_punch_hole = context.only_draw_punch_hole,
#ifndef NDEBUG
      .enable_debug_borders = context.enable_debug_borders,
      .enable_raster_cache_tag = context.enable_raster_cache_tag,
#endif
  };
  Begin(input);
  {
    auto mutator = input_stack.save();
    mutator.transform(transform);
    paint(input);
  }
  Finish(input);
  bounds_ = outer_bounds;
  root_transform_ = outer_root;
  StartRecording(context);
}

void DisplayScene::Finish(PaintContext& context) {
  context.state_stack.set_save_layer_handler({});
  FinishRecording(context);
}

bool DisplayScene::Prepare(const Rasterize& rasterize) {
  if (prepared_) return true;
  for (auto& entry : entries_) {
    if (entry.recording && entry.can_rasterize) {
      entry.image = rasterize(*entry.recording, entry.bounds);
      if (!entry.image) return false;
      entry.recording.reset();
    }
  }
  prepared_ = true;
  return true;
}

void DisplayScene::Paint(GrCanvas* canvas, GrContext* context,
                         std::optional<int64_t> slice) const {
  GrAutoCanvasRestore restore(canvas, true);
  CANVAS_CONCAT(canvas, inverse_root_);
  const auto base = CANVAS_GET_TOTAL_MATRIX(canvas);
  for (const auto& entry : entries_) {
    if (entry.slice == slice) entry.Paint(canvas, context, base);
  }
}

void DisplayScene::Entry::Paint(GrCanvas* canvas, GrContext* context,
                                const skity::Matrix& base) const {
  if (draw) {
    draw(canvas, context, base);
  } else if (recording) {
    recording->render_into(canvas);
  } else if (image) {
#ifndef ENABLE_SKITY
    canvas->drawImage(image, bounds.Left(), bounds.Top());
#else
    canvas->DrawImage(image, bounds.Left(), bounds.Top());
#endif
  }
}

std::optional<int64_t> DisplayScene::SwitchToView(
    PaintContext& context, std::optional<int64_t> view_id) {
  auto previous = current_slice_;
  if (previous == view_id) return previous;
  FinishRecording(context);
  // Every independently presented slice must have balanced group state.
  for (size_t i = 0; i < groups_.size(); ++i) {
    Entry close;
    close.draw = RestoreGroup;
    close.slice = previous;
    entries_.push_back(std::move(close));
  }
  current_slice_ = view_id;
  for (const auto& group : groups_) {
    Entry open;
    open.draw = group.resume;
    open.bounds = group.bounds;
    open.slice = view_id;
    entries_.push_back(std::move(open));
  }
  if (view_id && context.compositor_state) {
    context.compositor_state->GetSlices()[*view_id] =
        std::make_shared<DisplaySceneSlice>(shared_from_this(), *view_id,
                                            context.gr_context);
  }
  StartRecording(context);
  return previous;
}

std::list<skity::Rect> DisplayScene::SearchSlice(
    int64_t view_id, const skity::Rect& query) const {
  std::list<skity::Rect> rects;
  for (const auto& entry : entries_) {
    if (entry.slice != view_id || entry.bounds.IsEmpty()) continue;
    auto bounds = inverse_root_.MapRect(entry.bounds);
    if (bounds.Intersect(query)) rects.push_back(bounds);
  }
  return rects;
}

bool DisplayScene::ContainsImage(int64_t image_id) const {
  for (const auto& entry : entries_) {
    if (entry.update_image_id == image_id) return true;
  }
  return false;
}

}  // namespace clay
