// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/animation/basic_animation/basic_keyframe_effect.h"

#include <memory>
#include <utility>

#include "core/animation/basic_animation/animation_effect.h"
#include "core/animation/basic_animation/basic_animation.h"
#include "core/animation/basic_animation/keyframe.h"

namespace lynx {
namespace animation {
namespace basic {

KeyframeEffect::KeyframeEffect(
    std::vector<std::unique_ptr<KeyframeToken>> keyframes,
    const std::shared_ptr<AnimatorTarget>& target) {
  keyframes_token_map_ = std::move(keyframes);
  target_ = std::weak_ptr<AnimatorTarget>(target);
}

KeyframeEffect::KeyframeEffect(
    std::vector<std::unique_ptr<KeyframeToken>> keyframes,
    const std::shared_ptr<AnimatorTarget>& target,
    std::unique_ptr<AnimationEffectTiming> timing)
    : AnimationEffect(std::move(timing)) {
  keyframes_token_map_ = std::move(keyframes);
  target_ = std::weak_ptr<AnimatorTarget>(target);
}

KeyframeEffect::KeyframeEffect(
    std::vector<std::unique_ptr<KeyframeToken>> keyframes,
    const std::shared_ptr<AnimatorTarget>& target,
    std::unique_ptr<OptionalAnimationEffectTiming> timing)
    : AnimationEffect(std::move(timing)) {
  keyframes_token_map_ = std::move(keyframes);
  target_ = std::weak_ptr<AnimatorTarget>(target);
}

void clean_offset_func(std::vector<std::unique_ptr<KeyframeToken>>& vec) {
  // Get rid of keyframes with out-of-order offset.
  if (vec.empty()) return;

  bool all_non_null_opt = true;
  for (const auto& val : vec) {
    if (!val) {
      all_non_null_opt = false;
      break;
    }
  }
  if (all_non_null_opt) return;

  std::vector<std::unique_ptr<KeyframeToken>> cleaned_vec;
  std::optional<double> previous_value = std::nullopt;

  for (auto& val : vec) {
    if (val && val->offset()) {
      if (!previous_value || val->offset() > *previous_value) {
        previous_value = val->offset();
        cleaned_vec.push_back(std::move(val));
      }
    } else {
      cleaned_vec.push_back(std::move(val));
    }
  }

  vec = std::move(cleaned_vec);
}

void complete_sort_offset_func(
    std::vector<std::unique_ptr<KeyframeToken>>& vec) {
  if (vec.empty()) return;

  // Remove the first and last std::nullopt offset.
  if (!vec.front() && !vec.front()->offset()) {
    vec.erase(vec.begin());
  }
  if (!vec.empty() && !vec.back() && !vec.back()->offset()) {
    vec.pop_back();
  }

  if (vec.empty()) return;

  // Interpolate std::nullopt keyframes that lack an offset in the middle.
  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i] && !vec[i]->offset()) {
      // Find keyframes that are not std::nullopt before and after.
      size_t left = i;
      while (left > 0 && !vec[left]->offset()) {
        --left;
      }

      size_t right = i;
      while (right < vec.size() && !vec[right]->offset()) {
        ++right;
      }

      if (left < i && right < vec.size()) {
        double left_value = vec[left]->offset().value_or(0.0);
        double right_value = vec[right]->offset().value_or(0.0);
        for (size_t j = left + 1; j < right; ++j) {
          vec[j]->set_offset(left_value + (right_value - left_value) *
                                              (j - left) / (right - left));
        }
      }
    }
  }
}

void HandleOffsetSequence(std::vector<std::unique_ptr<KeyframeToken>>& vec) {
  clean_offset_func(vec);
  complete_sort_offset_func(vec);
}

void KeyframeEffect::MakeKeyframeModel() {
  HandleOffsetSequence(keyframes_token_map_);
  auto raw_ptr_timing_function = timing_->easing().release();
  for (const auto& keyframe_token : keyframes_token_map_) {
    // caculate offset if option is empty
    double offset = keyframe_token->offset().value_or(0.0);
    auto value_map = keyframe_token->property_values();
    if (value_map && value_map->empty()) {
      continue;
    }
    auto timing_function = keyframe_token->timing_function();
    for (auto& value_pair : *value_map) {
      if (value_pair.first == "animation-timing-function") {
        continue;
      }
      std::unique_ptr<Keyframe> keyframe = std::make_unique<Keyframe>(offset);
      keyframe->set_easing(timing_function);
      keyframe->AddPropertyValue(std::move(value_pair.second));
      auto iter = keyframe_models().find(value_pair.first);
      if (iter == keyframe_models().end()) {
        std::unique_ptr<AnimationCurve> new_curve =
            AnimationCurve::Create(value_pair.first, this);
        std::unique_ptr<KeyframeModel> model =
            KeyframeModel::Create(std::move(new_curve), this);
        iter = keyframe_models()
                   .insert({value_pair.first, std::move(model)})
                   .first;
      }
      iter->second->curve()->AddKeyframe(std::move(keyframe));
      iter->second->curve()->SetTimingFunction(raw_ptr_timing_function);
    }
  }
}

void KeyframeEffect::TickKeyframeModel(const fml::TimePoint& monotonic_time) {
  bool should_send_start_event = false;
  bool should_send_end_event = false;
  for (auto& model : keyframe_models_) {
    std::tie(should_send_start_event, should_send_end_event) =
        model.second->UpdateState(monotonic_time);

    if (!model.second->InEffect(monotonic_time)) {
      continue;
    }

    AnimationCurve* curve = model.second->curve();

    int temp_count = current_iteration_count_;

    fml::TimeDelta trimmed = model.second->TrimTimeToCurrentIteration(
        monotonic_time, current_iteration_count_);
    if (current_iteration_count_ != temp_count) {
      //      send iteration event
      HostAnimation()->SendAnimationEvent(Animation::EventType::Iteration);
    }
    std::unique_ptr<PropertyValue> property_value = curve->GetValue(trimmed);
    property_value_map_.insert_or_assign(curve->property_value_id(),
                                         std::move(property_value));
  }
  if (!property_value_map_.empty()) {
    auto strong_target = target_.lock();
    if (strong_target) {
      strong_target->UpdateAnimatedStyle(property_value_map_);
    }
    property_value_map_.clear();
  }

  if (should_send_start_event) {
    //    send start event
    HostAnimation()->SendAnimationEvent(Animation::EventType::Start);
    LOGI("Animation start, name is: ");
  }
  if (should_send_end_event) {
    //    send end event
    HostAnimation()->SendAnimationEvent(Animation::EventType::End);
    LOGI("Animation end, name is: ");
  }
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
