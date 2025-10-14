// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_ANIMATION_ANIMATION_MUTATOR_H_
#define CLAY_FLOW_ANIMATION_ANIMATION_MUTATOR_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "clay/common/element_id.h"
#include "clay/common/service/service.h"
#include "clay/flow/animation/scroll_offset_animation.h"
#include "clay/flow/services/animation_event_service.h"
#include "clay/gfx/animation/animator_target.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/gfx/geometry/transform_operations.h"
#ifndef ENABLE_SKITY
#include "clay/gfx/skia/picture_skia.h"
#endif
#include "skity/geometry/rect.hpp"
#include "skity/geometry/vector.hpp"

#ifdef ENABLE_SKITY
namespace clay {
class PictureSkity;
}  // namespace clay
#endif

namespace clay {

class PictureMutator;
class Layer;
class OpacityMutator;
class ScrollOffsetMutator;
class TransformMutator;

enum class AnimationMutatorType {
  kTransform,
  kOpacity,
  kPicture,
  kScrollOffset
};

// Stores animation information like Keyframes or Transition.
class AnimationMutator : public clay::AnimatorTarget,
                         public clay::AnimationEventHandler {
 public:
  AnimationMutator(const clay::ElementId& element_id, uint64_t layer_id);
  ~AnimationMutator();

  static std::shared_ptr<AnimationMutator> Create(
      const clay::ElementId& element_id, const AnimationMutatorType& type,
      Layer* layer);

  virtual AnimationMutatorType GetType() const = 0;
  virtual PictureMutator* asPicture() { return nullptr; }
  virtual OpacityMutator* asOpacity() { return nullptr; }
  virtual TransformMutator* asTransform() { return nullptr; }
  virtual ScrollOffsetMutator* asScrollOffset() { return nullptr; }

  uint64_t layer_id() const { return layer_id_; }

  clay::FloatSize PercentageResolutionSize() override { return {}; }

  const clay::ElementId& element_id() const { return element_id_; }

  void SetServiceManager(std::shared_ptr<clay::ServiceManager> service_manager);
  void ResetServiceManager();
  void AddTransitionManager(std::unique_ptr<clay::TransitionManager> manager);
  void AddKeyframesManager(std::unique_ptr<clay::KeyframesManager> manager);
  void SetScrollOffsetAnimation(
      std::shared_ptr<ScrollOffsetAnimation> animation);

  void SyncProperties(const std::shared_ptr<AnimationMutator> mutator);

  bool HasSameElementId(const std::shared_ptr<AnimationMutator> mutator) const;
  bool HasAnimationRunning() const;
  bool DoAnimationFrame(int64_t frame_time);

 protected:
  void OnAnimationEvent(const clay::AnimationParams& animation_params) override;
  void OnTransitionEvent(const clay::AnimationParams& animation_params,
                         ClayAnimationPropertyType property_type) override;

  clay::ElementId element_id_;
  uint64_t layer_id_ = 0;
  std::vector<std::unique_ptr<clay::KeyframesManager>> keyframes_managers_;
  std::vector<std::unique_ptr<clay::TransitionManager>> transition_managers_;
  std::shared_ptr<ScrollOffsetAnimation> scroll_offset_animation_;
  clay::Puppet<clay::Owner::kUI, AnimationEventService>
      animation_event_service_;
};

class TransformMutator : public AnimationMutator {
 public:
  TransformMutator(const clay::ElementId& element_id, uint64_t layer_id,
                   const clay::TransformOperations& transform)
      : AnimationMutator(element_id, layer_id), transform_(transform) {}

  AnimationMutatorType GetType() const override {
    return AnimationMutatorType::kTransform;
  }
  TransformMutator* asTransform() override { return this; }

  const clay::TransformOperations& transform() const { return transform_; }

 private:
  void GetProperty(ClayAnimationPropertyType type,
                   clay::TransformOperations& value) override;
  void SetProperty(ClayAnimationPropertyType type,
                   const clay::TransformOperations& value,
                   bool skip_update_for_raster_animation) override;

  clay::TransformOperations transform_;
};

class OpacityMutator : public AnimationMutator {
 public:
  OpacityMutator(const clay::ElementId& element_id, uint64_t layer_id,
                 float alpha)
      : AnimationMutator(element_id, layer_id), alpha_(alpha) {}

  AnimationMutatorType GetType() const override {
    return AnimationMutatorType::kOpacity;
  }
  OpacityMutator* asOpacity() override { return this; }
  float alpha() const { return alpha_ * 255; }

 private:
  void GetProperty(ClayAnimationPropertyType type, float& value) override;
  void SetProperty(ClayAnimationPropertyType type, float value,
                   bool skip_update_for_raster_animation) override;

  float alpha_;
};

class PictureMutator : public AnimationMutator {
 public:
  PictureMutator(const clay::ElementId& element_id, uint64_t layer_id,
#ifndef ENABLE_SKITY
                 clay::PictureSkia* picture
#else
                 clay::PictureSkity* picture
#endif  // ENABLE_SKITY
                 )
      : AnimationMutator(element_id, layer_id), picture_(picture) {
  }

  AnimationMutatorType GetType() const override {
    return AnimationMutatorType::kPicture;
  }
  PictureMutator* asPicture() override { return this; }

 private:
  void GetProperty(ClayAnimationPropertyType type, clay::Color& value) override;
  void SetProperty(ClayAnimationPropertyType type, const clay::Color& value,
                   bool skip_update_for_raster_animation) override;

#ifndef ENABLE_SKITY
  clay::PictureSkia* picture_;
#else
  clay::PictureSkity* picture_;
#endif  // ENABLE_SKITY
};

class ScrollOffsetMutator : public AnimationMutator {
 public:
  ScrollOffsetMutator(const clay::ElementId& element_id, uint64_t layer_id,
                      const skity::Matrix& transform)
      : AnimationMutator(element_id, layer_id), transform_(transform) {}
  AnimationMutatorType GetType() const override {
    return AnimationMutatorType::kScrollOffset;
  }
  ScrollOffsetMutator* asScrollOffset() override { return this; }

  void Initialize(const skity::Vec2& offset, const skity::Vec2& scroll_offset,
                  const skity::Rect& visible_offset_range,
                  const skity::Rect& max_offset_range);

  const skity::Vec2& GetScrollOffset() const;
  const skity::Rect& GetVisibleOffsetRange() const;
  const skity::Rect& GetMaxOffsetRange() const;
  const skity::Matrix& transform() const { return transform_; }

  void UpdateScrollOffset(const skity::Vec2& scroll_offset);

  // The scroll animation events sent by ScrollOffsetAnimation, and will be sent
  // to UI layer by `animation_event_service_`.
  void OnScrolled(int32_t session_id, float scroll_offset,
                  bool ignore_ui_repaint);
  void OnScrollEnd(int32_t session_id, float scroll_offset, float velocity);

 private:
  skity::Matrix transform_;
  // Static offset from layout includes border, padding and margin. It's
  // immutable.
  skity::Vec2 offset_;
  // Scroll offset, it's mutable and may be changed by raster animation.
  skity::Vec2 scroll_offset_;
  skity::Rect visible_offset_range_;
  skity::Rect max_offset_range_;
};

// A AnimationMutators owns a stack of all Keyframes and Transition attached to
// a single target Layer.
using AnimationMutators =
    std::unordered_map<uint64_t, std::shared_ptr<AnimationMutator>>;

}  // namespace clay

#endif  // CLAY_FLOW_ANIMATION_ANIMATION_MUTATOR_H_
