// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/compositing/frame_builder.h"

#include <memory>
#include <utility>
#include <vector>

#include "clay/common/element_id.h"
#include "clay/flow/animation/animation_host.h"
#include "clay/flow/animation/animation_mutator.h"
#include "clay/flow/layers/backdrop_filter_layer.h"
#include "clay/flow/layers/clip_path_layer.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/clip_rrect_layer.h"
#include "clay/flow/layers/color_filter_layer.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/drawable_image_layer.h"
#include "clay/flow/layers/external_view_layer.h"
#include "clay/flow/layers/image_filter_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/layers/performance_overlay_layer.h"
#include "clay/flow/layers/picture_layer.h"
#include "clay/flow/layers/platform_view_layer.h"
#include "clay/flow/layers/punch_hole_layer.h"
#include "clay/flow/layers/shader_mask_layer.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/fml/logging.h"
#include "clay/gfx/picture.h"
#include "clay/public/style_types.h"
#include "clay/ui/compositing/pending_layer.h"
#include "clay/ui/compositing/pending_picture_layer.h"
#include "clay/ui/rendering/render_object.h"
#include "skity/geometry/vector.hpp"

namespace clay {
namespace {

void InitializeAnimationMutator(RenderObject* owner,
                                clay::AnimationMutator* mutator,
                                ClayAnimationPropertyType type) {
  // For Keyframe animation has a higher priority than Transition animation
  // in the CSS style rules.
  if (owner->HasAnimation(type)) {
    FML_DCHECK(owner->GetKeyframesManager() != nullptr);
    if (auto manager = owner->GetKeyframesManager()->CloneForRasterAnimation(
            type, mutator)) {
      manager->SetEventHandler(mutator);
      mutator->AddKeyframesManager(std::move(manager));
    }
  } else if (owner->HasTransition(type)) {
    FML_DCHECK(owner->GetTransitionManager() != nullptr);
    if (auto manager = owner->GetTransitionManager()->CloneForRasterAnimation(
            type, mutator)) {
      manager->SetEventHandler(mutator);
      mutator->AddTransitionManager(std::move(manager));
    }
  }
}

}  // namespace

FrameBuilder::FrameBuilder(const skity::Vec2& frame_size,
                           float device_pixel_ratio,
                           fml::RefPtr<GPUUnrefQueue> unref_queue)
    : frame_size_(frame_size),
      device_pixel_ratio_(device_pixel_ratio),
      unref_queue_(std::move(unref_queue)) {
  // Add a ContainerLayer as the root layer, so that AddLayer operations are
  // always valid.
  PushLayer(std::make_shared<clay::ContainerLayer>());
}

FrameBuilder::~FrameBuilder() = default;

void FrameBuilder::Reset() {
  // Clear the composited layer stack.
  layer_stack_.clear();
  // Add a TransformLayer as the root layer again.
  PushLayer(std::make_shared<clay::TransformLayer>());
}

void FrameBuilder::UpdateFrameSize(const skity::Vec2& size,
                                   float device_pixel_ratio) {
  if (size == frame_size_ && device_pixel_ratio == device_pixel_ratio_) {
    return;
  }
  // TODO(jinsong): frame size and device pixel ratio has been changed, need to
  // rebuild frame.
  layer_tree_.reset();
  frame_size_ = size;
  device_pixel_ratio_ = device_pixel_ratio;
}

std::unique_ptr<Picture> FrameBuilder::GeneratePicture(
    const skity::Rect& bounds) {
  return nullptr;
}

void FrameBuilder::BuildFrame(PendingLayer* root_layer) {
  if (!root_layer) {
    layer_tree_.reset();
    return;
  }
  FML_DCHECK(!root_layer->Parent());
  animation_host_ = std::make_shared<clay::AnimationHost>();
  root_layer->UpdateSubtreeNeedsAddToFrame();
  root_layer->AddToFrame(this);
  root_layer->need_add_to_frame_ = false;
  FinishBuild();
}

void FrameBuilder::BuildSubtreeFrame(PendingLayer* root_layer) {
  // The root layer may be a sub layer of the current page.
  // Use this function to avoid dcheck failure.
  if (!root_layer) {
    layer_tree_.reset();
    return;
  }
  animation_host_ = std::make_shared<clay::AnimationHost>();
  root_layer->UpdateSubtreeNeedsAddToFrame();
  root_layer->AddToFrame(this);
  root_layer->need_add_to_frame_ = false;
  FinishBuild();
}

void FrameBuilder::FinishBuild() {
  if (frame_size_.x == 0 && frame_size_.y == 0) {
    layer_tree_.reset();
    return;
  }
  // The layer tree has been constructed and is ready to be submitted to
  // Engine.
  layer_tree_ =
      std::make_unique<clay::LayerTree>(frame_size_, device_pixel_ratio_);
  layer_tree_->set_root_layer(RootLayer());
  layer_tree_->SetAnimationHost(std::move(animation_host_));
}

void FrameBuilder::PushTransformOperations(const TransformOperations& transform,
                                           double origin_x, double origin_y,
                                           double offset_x, double offset_y,
                                           PendingLayer* old_layer) {
  auto layer = std::make_shared<clay::TransformLayer>(
      transform, skity::Vec2(origin_x, origin_y),
      skity::Vec2(offset_x, offset_y));
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);

  PendingLayer* owned_layer = FindOwnedLayer(old_layer);
  if (owned_layer && owned_layer->owner_->HasRasterAnimation()) {
    RenderObject* owner = owned_layer->owner_;
    auto mutator = clay::AnimationMutator::Create(
        owner->element_id(), clay::AnimationMutatorType::kTransform,
        layer.get());
    InitializeAnimationMutator(owner, mutator.get(),
                               ClayAnimationPropertyType::kTransform);
    animation_host_->AddAnimationMutator(layer.get(), std::move(mutator));
  }
}

void FrameBuilder::PushStaticTransform(skity::Matrix transform,
                                       PendingLayer* old_layer) {
  auto layer = std::make_shared<clay::TransformLayer>(transform);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushOffset(double dx, double dy, PendingLayer* old_layer) {
  skity::Matrix sk_matrix = skity::Matrix::Translate(dx, dy);
  auto layer = std::make_shared<clay::TransformLayer>(sk_matrix);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushScrollOffset(
    double x, double y, double scroll_x, double scroll_y,
    const FloatRect& offset_range, const FloatRect& max_offset_range,
    std::shared_ptr<clay::ScrollOffsetAnimation> animation,
    PendingLayer* old_layer) {
  skity::Matrix sk_matrix =
      skity::Matrix::Translate(x + scroll_x, y + scroll_y);
  auto layer = std::make_shared<clay::TransformLayer>(sk_matrix);
  PushLayer(layer);
  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);

  PendingLayer* owned_layer = FindOwnedLayer(old_layer);
  if (owned_layer) {
    RenderObject* owner = owned_layer->owner_;
    auto mutator = clay::AnimationMutator::Create(
        owner->element_id(), clay::AnimationMutatorType::kScrollOffset,
        layer.get());
    mutator->asScrollOffset()->Initialize(skity::Vec2{x, y},
                                          skity::Vec2{scroll_x, scroll_y},
                                          offset_range, max_offset_range);
    // Bind ScrollOffsetAnimation.
    mutator->SetScrollOffsetAnimation(std::move(animation));
    animation_host_->AddAnimationMutator(layer.get(), std::move(mutator));
  }
}

PendingLayer* FrameBuilder::FindOwnedLayer(PendingLayer* layer) const {
  while (layer) {
    if (layer->owner_) {
      return layer;
    }
    layer = static_cast<PendingLayer*>(layer->Parent());
  }
  return nullptr;
}

void FrameBuilder::PushOpacity(int alpha, double dx, double dy,
                               PendingLayer* old_layer) {
  auto layer = std::make_shared<clay::OpacityLayer>(alpha, skity::Vec2(dx, dy));
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);

  PendingLayer* owned_layer = FindOwnedLayer(old_layer);
  if (owned_layer && owned_layer->owner_->HasRasterAnimation()) {
    RenderObject* owner = owned_layer->owner_;
    auto mutator = clay::AnimationMutator::Create(
        owner->element_id(), clay::AnimationMutatorType::kOpacity, layer.get());
    InitializeAnimationMutator(owner, mutator.get(),
                               ClayAnimationPropertyType::kOpacity);
    animation_host_->AddAnimationMutator(layer.get(), std::move(mutator));
  }
}

void FrameBuilder::PushColorFilter(std::shared_ptr<ColorFilter> color_filter,
                                   PendingLayer* old_layer) {
  auto layer = std::make_shared<clay::ColorFilterLayer>(color_filter);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushImageFilter(std::shared_ptr<ImageFilter> image_filter,
                                   PendingLayer* old_layer) {
  auto layer = std::make_shared<clay::ImageFilterLayer>(image_filter);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushShaderMask(std::shared_ptr<ColorSource> color_source,
                                  const FloatRect& mask_rect,
                                  BlendMode blend_mode,
                                  PendingLayer* old_layer) {
  auto layer = std::make_shared<clay::ShaderMaskLayer>(color_source, mask_rect,
                                                       blend_mode);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushBackdropFilter(std::shared_ptr<ImageFilter> filter,
                                      PendingLayer* old_layer) {
  auto layer =
      std::make_shared<clay::BackdropFilterLayer>(filter, BlendMode::kSrcOver);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushClipRect(const FloatRect& clip_rect, int clip_behavior,
                                PendingLayer* old_layer) {
  clay::Clip behavior = static_cast<clay::Clip>(clip_behavior);
  auto layer = std::make_shared<clay::ClipRectLayer>(clip_rect, behavior);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushClipRRect(const FloatRoundedRect& clip_rrect,
                                 int clip_behavior, PendingLayer* old_layer) {
  clay::Clip behavior = static_cast<clay::Clip>(clip_behavior);
  auto layer = std::make_shared<clay::ClipRRectLayer>(clip_rrect, behavior);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::PushClipPath(const GrPath& path, int clip_behavior,
                                PendingLayer* old_layer) {
  clay::Clip behavior = static_cast<clay::Clip>(clip_behavior);
  auto layer = std::make_shared<clay::ClipPathLayer>(path, behavior);
  PushLayer(layer);

  if (old_layer->ReuseLayer()) {
    layer->AssignOldLayer(old_layer->ReuseLayer().get());
  }
  old_layer->RetainLayer(layer);
}

void FrameBuilder::Pop() { PopLayer(); }

void FrameBuilder::AddPicture(double dx, double dy,
                              PendingPictureLayer* picture_layer,
                              bool complex_hint, bool change_hint,
                              CacheStrategy strategy) {
  FML_DCHECK(picture_layer->picture());
  auto picture = picture_layer->picture()->picture();
  if (picture) {
    auto layer = std::make_unique<clay::PictureLayer>(
        skity::Vec2(dx, dy), CreateGPUObject(picture), complex_hint,
        change_hint, strategy, picture_layer->picture()->HasLazyImage());
    if (!picture->GetDynamicOps().empty()) {
      FML_DCHECK(picture_layer->Parent());
      auto* container_layer =
          static_cast<PendingLayer*>(picture_layer->Parent());
      FML_DCHECK(container_layer->Owner());
      // PictureLayer does not have an owner, but its parent which should be a
      // `ContainerLayer` is supposed to have one.
      RenderObject* owner = container_layer->Owner();
      auto mutator = clay::AnimationMutator::Create(
          owner->element_id(), clay::AnimationMutatorType::kPicture,
          layer.get());
      InitializeAnimationMutator(owner, mutator.get(),
                                 ClayAnimationPropertyType::kBackgroundColor);
      InitializeAnimationMutator(owner, mutator.get(),
                                 ClayAnimationPropertyType::kColor);
      animation_host_->AddAnimationMutator(layer.get(), std::move(mutator));
    }
    AddLayer(std::move(layer));
  }
}

void FrameBuilder::AddDrawableImage(double dx, double dy, double width,
                                    double height, int64_t image_id,
                                    clay::DrawableImage::FitMode fit_mode) {
  auto layer = std::make_unique<clay::DrawableImageLayer>(
      skity::Vec2(dx, dy), skity::Vec2(width, height), image_id, false,
      ImageSampling::kLinear, fit_mode);
  AddLayer(std::move(layer));
}

void FrameBuilder::AddPlatformView(double dx, double dy, double width,
                                   double height, int64_t view_id) {
  auto layer = std::make_unique<clay::PlatformViewLayer>(
      skity::Vec2(dx, dy), skity::Vec2(width, height), view_id);
  AddLayer(std::move(layer));
}

void FrameBuilder::AddPunchHole(const skity::Rect& rect) {
  auto layer = std::make_unique<clay::PunchHoleLayer>(rect);
  AddLayer(std::move(layer));
}

void FrameBuilder::AddPerformanceOverlay(uint64_t enable_options, double left,
                                         double right, double top,
                                         double bottom) {
  skity::Rect rect = skity::Rect::MakeLTRB(left, top, right, bottom);
  auto layer = std::make_unique<clay::PerformanceOverlayLayer>(enable_options);
  layer->set_paint_bounds(rect);
  AddLayer(std::move(layer));
}

std::shared_ptr<clay::Layer> FrameBuilder::RootLayer() const {
  FML_DCHECK(layer_stack_.size() >= 1);

  return layer_stack_[0];
}

std::unique_ptr<clay::LayerTree> FrameBuilder::TakeLayerTree() {
  return std::move(layer_tree_);
}

void FrameBuilder::AddLayer(std::shared_ptr<clay::Layer> layer) {
  FML_DCHECK(layer);

  if (!layer_stack_.empty()) {
    layer_stack_.back()->Add(std::move(layer));
  }
}

void FrameBuilder::PushLayer(std::shared_ptr<clay::ContainerLayer> layer) {
  AddLayer(layer);
  layer_stack_.push_back(std::move(layer));
}

void FrameBuilder::PopLayer() {
  // We never pop the root layer, so that AddLayer operations are always valid.
  if (layer_stack_.size() > 1) {
    layer_stack_.pop_back();
  }
}

void FrameBuilder::AddRetained(std::shared_ptr<clay::Layer> engine_layer) {
  AddLayer(engine_layer);
  CopyRasterAnimationsFromRetained(engine_layer);
}

void FrameBuilder::CopyRasterAnimationsFromRetained(
    const std::shared_ptr<clay::Layer>& layer) {
  if (layer->HasAnimation()) {
    animation_host_->AddRetainedLayerId(layer->unique_id());
  }
  if (layer->IsContainer()) {
    for (auto& child :
         std::static_pointer_cast<clay::ContainerLayer>(layer)->layers()) {
      CopyRasterAnimationsFromRetained(child);
    }
  }
}

void FrameBuilder::PushExternalViewLayer(const ElementId& element_id,
                                         const skity::Vec2& size) {
  auto layer = std::make_unique<clay::ExternalViewLayer>(element_id, size);
  PushLayer(std::move(layer));
}

#ifndef NDEBUG
void FrameBuilder::DumpLayerTree() const {
  FML_LOG(ERROR) << "frame_size_=(" << frame_size_.x << "," << frame_size_.y
                 << ") "
                 << "device_pixel_ratio_=" << device_pixel_ratio_;
  if (layer_tree_.get() && layer_tree_->root_layer()) {
    layer_tree_->root_layer()->DebugDumpTree(0);
  } else if (layer_stack_.size() > 0) {
    layer_stack_[0]->DebugDumpTree(0);
  }
}
#endif

}  // namespace clay
