// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_H_
#define CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_H_

#include <functional>
#include <memory>
#include <string>

#include "core/animation/basic_animation/animation_effect.h"
#include "core/animation/basic_animation/animation_effect_timing.h"
#include "core/animation/basic_animation/animation_frame_callback.h"
#include "core/animation/basic_animation/animation_frame_callback_provider.h"
#include "core/animation/basic_animation/basic_animation.h"
#include "core/animation/basic_animation/basic_keyframe_effect.h"
#include "core/animation/lynx_basic_animator/basic_animator_event_listener.h"
#include "core/animation/lynx_basic_animator/basic_animator_frame_callback_provider.h"
#include "core/animation/lynx_basic_animator/basic_property_value.h"
#include "core/style/animation_data.h"
namespace lynx {

namespace shell {
class VSyncMonitor;
}

namespace animation {
namespace basic {

class BasicAnimatorFrameCallbackProvider;

class LynxBasicAnimator : public basic::AnimatorTarget {
 public:
  enum BasicValueType { INT = 0, FLOAT };
  using Callback = std::function<void(float)>;

  LynxBasicAnimator(
      starlight::AnimationData data,
      std::shared_ptr<shell::VSyncMonitor> vsync_monitor = nullptr,
      BasicValueType type = BasicValueType::FLOAT)
      : basic_type_(type), data_(data) {
    frame_provider_ =
        std::make_shared<BasicAnimatorFrameCallbackProvider>(vsync_monitor);
    basic_animator_event_listener_ =
        std::make_shared<BasicAnimatorEventListener>();
  };

  void InitializeAnimator();

  void RegisterCustomCallback(const Callback& cb) { custom_callback_ = cb; }

  void RegisterEventCallback(
      const BasicAnimatorEventListener::EventCallback& cb,
      Animation::EventType event_type);

  void Start() {
    this->InitializeAnimator();
    animation_->Play();
  }

  void Stop() {
    if (animation_) {
      animation_->Stop();
    }
  }

  void UpdateAnimatedStyle(const Keyframe::PropertyValueMap& styles) override;

  std::unique_ptr<basic::PropertyValue> GetStyle(
      const std::string& property_name) override {
    // TODO: need return invalid value
    return std::make_unique<BasicFloatPropertyValue>(1.0);
  }

  BasicValueType basic_type() { return basic_type_; }

 private:
  BasicValueType basic_type_;
  starlight::AnimationData data_;
  std::shared_ptr<basic::Animation> animation_;
  std::shared_ptr<BasicAnimatorFrameCallbackProvider> frame_provider_;
  std::shared_ptr<BasicAnimatorEventListener> basic_animator_event_listener_;
  // The animator executes a callback on each frame, and passed by the user.
  Callback custom_callback_;
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_H_
