// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/lynx_basic_animator/basic_animator.h"

#include <utility>
#include <vector>

namespace lynx {
namespace animation {
namespace basic {

void LynxBasicAnimator::UpdateAnimatedStyle(
    const Keyframe::PropertyValueMap &styles) {
  auto progress_iter = styles.find("BASIC_TYPE_FLOAT");
  if (progress_iter != styles.end()) {
    auto progress =
        static_cast<BasicFloatPropertyValue *>(progress_iter->second.get())
            ->GetFloatValue();
    if (custom_callback_) {
      return custom_callback_(progress);
    } else {
      LOGE("LynxBasicAnimator has no custom callback! Please check!")
    }
  } else {
    LOGE("A normal progress is not returned here");
  }
}

void LynxBasicAnimator::InitializeAnimator() {
  std::vector<std::unique_ptr<basic::KeyframeToken>> keyframes;

  std::unique_ptr<basic::KeyframeToken> from_keyframe_token =
      std::make_unique<basic::KeyframeToken>(0.0);
  std::unique_ptr<basic::PropertyValue> from_property_value =
      std::make_unique<BasicFloatPropertyValue>(0.0);
  from_keyframe_token->AddPropertyValueForToken("BASIC_TYPE_FLOAT",
                                                std::move(from_property_value));
  keyframes.emplace_back(std::move(from_keyframe_token));

  std::unique_ptr<basic::KeyframeToken> to_keyframe_token =
      std::make_unique<basic::KeyframeToken>(1.0);
  std::unique_ptr<basic::PropertyValue> to_property_value =
      std::make_unique<BasicFloatPropertyValue>(1.0);
  to_keyframe_token->AddPropertyValueForToken("BASIC_TYPE_FLOAT",
                                              std::move(to_property_value));
  keyframes.emplace_back(std::move(to_keyframe_token));

  std::unique_ptr<basic::AnimationEffectTiming> effect_timing =
      basic::AnimationEffectTiming::Create(
          fml::TimeDelta::FromMillisecondsF(data_.delay),
          static_cast<basic::AnimationEffectTiming::FillMode>(data_.fill_mode),
          data_.iteration_count,
          fml::TimeDelta::FromMillisecondsF(data_.duration),
          static_cast<basic::AnimationEffectTiming::PlaybackDirection>(
              data_.direction),
          TimingFunction::MakeTimingFunction(data_.timing_func));
  std::unique_ptr<basic::KeyframeEffect> effect = basic::KeyframeEffect::Create(
      std::move(keyframes), shared_from_this(), std::move(effect_timing));
  animation_ = std::make_shared<basic::Animation>(std::move(effect));
  animation_->RegisterAnimationFrameCallbackProvider(frame_provider_);
  animation_->AddEventListener(basic_animator_event_listener_);
};

void LynxBasicAnimator::RegisterEventCallback(
    const BasicAnimatorEventListener::EventCallback &cb,
    Animation::EventType event_type) {
  switch (event_type) {
    case Animation::EventType::Start:
      basic_animator_event_listener_->RegisterStartCallback(cb);
      break;
    case Animation::EventType::Iteration:
      basic_animator_event_listener_->RegisterIterationCallback(cb);
      break;
    case Animation::EventType::Cancel:
      basic_animator_event_listener_->RegisterCancelCallback(cb);
      break;
    case Animation::EventType::End:
      basic_animator_event_listener_->RegisterEndCallback(cb);
    default:
      LOGE("There is no event of this type");
      break;
  }
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
