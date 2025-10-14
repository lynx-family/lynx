// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/animation/animation_mutator.h"

#include <utility>

#include "clay/common/service/service_manager.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/layers/picture_layer.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/fml/logging.h"

namespace clay {

// static
std::shared_ptr<AnimationMutator> AnimationMutator::Create(
    const clay::ElementId& element_id, const AnimationMutatorType& type,
    Layer* layer) {
  switch (type) {
    case AnimationMutatorType::kTransform: {
      auto mutator = std::make_shared<TransformMutator>(
          element_id, layer->unique_id(),
          static_cast<TransformLayer*>(layer)->GetTransform());
      return std::move(mutator);
    }
    case AnimationMutatorType::kOpacity: {
      float alpha = static_cast<OpacityLayer*>(layer)->opacity();
      auto mutator = std::make_shared<OpacityMutator>(
          element_id, layer->unique_id(), alpha);
      return std::move(mutator);
    }
    case AnimationMutatorType::kPicture: {
      auto mutator = std::make_shared<PictureMutator>(
          element_id, layer->unique_id(),
#ifndef ENABLE_SKITY
          static_cast<PictureLayer*>(layer)->picture_skia());
#else
          static_cast<PictureLayer*>(layer)->picture_skity());
#endif  // ENABLE_SKITY
      return std::move(mutator);
    }
    case AnimationMutatorType::kScrollOffset: {
      auto mutator = std::make_shared<ScrollOffsetMutator>(
          element_id, layer->unique_id(),
          static_cast<TransformLayer*>(layer)->GetMatrix());
      return std::move(mutator);
    }
    default:
      FML_UNREACHABLE();
  }
}

AnimationMutator::AnimationMutator(const clay::ElementId& element_id,
                                   uint64_t layer_id)
    : element_id_(element_id), layer_id_(layer_id) {}

AnimationMutator::~AnimationMutator() {
  // Must reset keyframes manager and transition manager before
  // destroying the puppet. Since the destructor of keyframes manager
  // may call back to `this` and its puppet.
  keyframes_managers_.clear();
  transition_managers_.clear();
  animation_event_service_ = nullptr;
}

bool AnimationMutator::HasSameElementId(
    const std::shared_ptr<AnimationMutator> mutator) const {
  return mutator && mutator->element_id() == element_id_;
}

void AnimationMutator::AddTransitionManager(
    std::unique_ptr<clay::TransitionManager> manager) {
  transition_managers_.push_back(std::move(manager));
}

void AnimationMutator::AddKeyframesManager(
    std::unique_ptr<clay::KeyframesManager> manager) {
  keyframes_managers_.push_back(std::move(manager));
}

void AnimationMutator::SetScrollOffsetAnimation(
    std::shared_ptr<ScrollOffsetAnimation> animation) {
  scroll_offset_animation_ = std::move(animation);
}

void AnimationMutator::SyncProperties(
    const std::shared_ptr<AnimationMutator> mutator) {
  if (!mutator) {
    return;
  }
  for (auto& mgr : keyframes_managers_) {
    for (auto& old_mgr : mutator->keyframes_managers_) {
      mgr->SyncProperties(old_mgr.get());
    }
  }

  for (auto& mgr : transition_managers_) {
    for (auto& old_mgr : mutator->transition_managers_) {
      mgr->SyncProperties(old_mgr.get());
    }
  }
}

bool AnimationMutator::HasAnimationRunning() const {
  for (auto& mgr : transition_managers_) {
    if (mgr->HasAnimationRunning()) {
      return true;
    }
  }
  for (auto& mgr : keyframes_managers_) {
    if (mgr->HasAnimationRunning()) {
      return true;
    }
  }
  if (scroll_offset_animation_) {
    return scroll_offset_animation_->IsRunning();
  }
  return false;
}

bool AnimationMutator::DoAnimationFrame(int64_t frame_time) {
  bool stopped = true;
  for (auto& mgr : transition_managers_) {
    for (auto& animator : mgr->GetRunningAnimators()) {
      stopped &= animator->DoAnimationFrame(frame_time);
    }
  }
  for (auto& mgr : keyframes_managers_) {
    for (auto& animation : mgr->animations()) {
      if (animation.animator->IsStarted()) {
        stopped &= animation.animator->DoAnimationFrame(frame_time);
      }
    }
  }
  if (scroll_offset_animation_) {
    stopped &= scroll_offset_animation_->DoAnimationFrame(frame_time, this);
  }
  return stopped;
}

void AnimationMutator::SetServiceManager(
    std::shared_ptr<clay::ServiceManager> service_manager) {
  if (!animation_event_service_) {
    animation_event_service_ =
        service_manager->GetService<AnimationEventService>();
  }
}

void AnimationMutator::ResetServiceManager() {
  animation_event_service_ = nullptr;
}

void AnimationMutator::OnAnimationEvent(
    const clay::AnimationParams& animation_params) {
  if (animation_event_service_) {
    animation_event_service_->OnAnimationEvent(element_id_, animation_params);
  }
}

void AnimationMutator::OnTransitionEvent(
    const clay::AnimationParams& animation_params,
    ClayAnimationPropertyType property_type) {
  if (animation_event_service_) {
    animation_event_service_->OnTransitionEvent(element_id_, animation_params,
                                                property_type);
  }
}

void TransformMutator::GetProperty(ClayAnimationPropertyType type,
                                   clay::TransformOperations& value) {
  if (type == ClayAnimationPropertyType::kTransform) {
    value = transform_;
  }
}

void TransformMutator::SetProperty(ClayAnimationPropertyType type,
                                   const clay::TransformOperations& value,
                                   bool skip_update_for_raster_animation) {
  if (type == ClayAnimationPropertyType::kTransform) {
    transform_ = value;
  }
}

void OpacityMutator::GetProperty(ClayAnimationPropertyType type, float& value) {
  if (type == ClayAnimationPropertyType::kOpacity) {
    value = alpha_;
  }
}

void OpacityMutator::SetProperty(ClayAnimationPropertyType type, float value,
                                 bool skip_update_for_raster_animation) {
  if (type == ClayAnimationPropertyType::kOpacity) {
    alpha_ = value;
  }
}

void PictureMutator::GetProperty(ClayAnimationPropertyType type,
                                 clay::Color& value) {
  if (!picture_) {
    return;
  }
  value = picture_->ObtainWorkletValue(type);
}

void PictureMutator::SetProperty(ClayAnimationPropertyType type,
                                 const clay::Color& value,
                                 bool skip_update_for_raster_animation) {
  if (!picture_) {
    return;
  }
  picture_->DispatchToWorklet(type, value);
}

void ScrollOffsetMutator::Initialize(const skity::Vec2& offset,
                                     const skity::Vec2& scroll_offset,
                                     const skity::Rect& visible_offset_range,
                                     const skity::Rect& max_offset_range) {
  offset_ = offset;
  scroll_offset_ = scroll_offset;
  visible_offset_range_ = visible_offset_range;
  max_offset_range_ = max_offset_range;
}

const skity::Vec2& ScrollOffsetMutator::GetScrollOffset() const {
  return scroll_offset_;
}

const skity::Rect& ScrollOffsetMutator::GetVisibleOffsetRange() const {
  return visible_offset_range_;
}

const skity::Rect& ScrollOffsetMutator::GetMaxOffsetRange() const {
  return max_offset_range_;
}

void ScrollOffsetMutator::UpdateScrollOffset(const skity::Vec2& scroll_offset) {
  scroll_offset_ = scroll_offset;
  transform_ = skity::Matrix::Translate(offset_.x + scroll_offset_.x,
                                        offset_.y + scroll_offset_.y);
}

void ScrollOffsetMutator::OnScrolled(int32_t session_id, float scroll_offset,
                                     bool ignore_ui_repaint) {
  // Synchronize scrolled event to UI thread by AnimationEventService.
  if (animation_event_service_) {
    animation_event_service_->OnScrolled(element_id_, session_id, scroll_offset,
                                         ignore_ui_repaint);
  }
}

void ScrollOffsetMutator::OnScrollEnd(int32_t session_id, float scroll_offset,
                                      float velocity) {
  // Synchronize scroll end event to UI thread by AnimationEventService.
  if (animation_event_service_) {
    animation_event_service_->OnScrollEnd(element_id_, session_id,
                                          scroll_offset, velocity);
  }
}

}  // namespace clay
