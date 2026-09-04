// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_ANIMATION_H_
#define CORE_ANIMATION_ANIMATION_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>

#include "base/include/fml/time/time_point.h"
#include "core/animation/keyframe_effect.h"
#include "core/base/lynx_export.h"

namespace lynx {
namespace base {
class VSyncMonitor;
}

namespace tasm {
class Element;
class CSSKeyframesToken;
}  // namespace tasm

namespace animation {
class KeyframeEffect;
class Animation : public std::enable_shared_from_this<Animation> {
 public:
  // It is a dummy animation start time used to indicate that the starting time
  // for the animation has not yet been properly set.
  // Q: Why do we need this dummy time?
  // A: This dummy time is used to immediately tick the animation when it is
  // created to ensure the style is correct. When the next vsync arrives, the
  // correct frame time should be used to update the animation's start time.

  // TODO(wujintian): Mark the fml::TimePoint parameter as const in all
  // interfaces of animation, and then mark this variable as const.
  static fml::TimePoint& GetAnimationDummyStartTime();

  enum class State { kIdle = 0, kPlay, kPause, kStop };

  // Origin category used by the CDP Animation domain.
  enum class Origin : uint8_t {
    // Animation created from an author-defined CSS @keyframes rule.
    kCSSAnimation = 0,
    // Animation created by the CSS transition manager.
    kCSSTransition,
    // Animation created through Lynx's imperative Animate or AnimateV2 API.
    kWebAnimation,
  };

  Animation(const base::String& name);
  ~Animation() = default;
  void Play(bool play_handles_initial_frame = true);
  void Pause();
  void Stop();
  void Destroy(bool need_clear_effect = true);

  void DoFrame(fml::TimePoint& frame_time);
  KeyframeEffect::KeyframeSampleResult SampleAt(fml::TimePoint& frame_time);

  void SendStartEvent();

  void SendEndEvent();

  void SendCancelEvent();

  void SendIterationEvent();

  const base::String& name() { return name_; }

  // Stable process-unique identifier used as CDP Animation.id.
  int64_t id() const { return id_; }

  // Returns the current timeline time without mutating animation state.
  LYNX_EXPORT_FOR_DEVTOOL fml::TimeDelta GetCurrentTime() const;

  const fml::TimePoint& start_time() const { return start_time_; }
  const fml::TimePoint& pause_time() const { return pause_time_; }
  const fml::TimeDelta& total_paused_duration() const {
    return total_paused_duration_;
  }

  void BindDelegate(AnimationDelegate* target);

  void SetKeyframeEffect(std::unique_ptr<KeyframeEffect> keyframe_effect);

  KeyframeEffect* keyframe_effect() { return keyframe_effect_.get(); }

  void BindElement(tasm::Element* element) { element_ = element; }

  tasm::Element* GetElement() { return element_; }

  void set_animation_data(starlight::AnimationData& data) {
    animation_data_ = data;
  }

  starlight::AnimationData& get_animation_data() { return animation_data_; }

  void UpdateAnimationData(starlight::AnimationData& data);

  void UpdateUnderlyingValue(AnimationCurve::CurveType type,
                             const tasm::CSSValue& value);

  starlight::AnimationData* animation_data() { return &animation_data_; }
  const starlight::AnimationData* animation_data() const {
    return &animation_data_;
  }

  void SetRawCssId(tasm::CSSPropertyID id) { raw_style_set_.insert(id); }

  std::unordered_set<tasm::CSSPropertyID>& GetRawStyleSet() {
    return raw_style_set_;
  }

  void SetRawCustomProperty(const base::String& name) {
    raw_custom_property_set_.insert(name);
  }

  void ClearRawCustomProperties() { raw_custom_property_set_.clear(); }

  std::unordered_set<base::String>& GetRawCustomPropertySet() {
    return raw_custom_property_set_;
  }

  State GetState() const { return state_; }

  void SetTransitionFlag() { is_transition_ = true; }

  bool GetTransitionFlag() { return is_transition_; }

  Origin GetOrigin() const { return origin_; }

  void SetOrigin(Origin origin) { origin_ = origin; }

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

  void ClearTransitionPreviousEndValue();

 protected:
  fml::TimePoint start_time_{fml::TimePoint::Min()};

 private:
  friend class KeyframeEffect;

  void MaybeReportOverTime(fml::TimeDelta active_time);
  void ReportAnimationOverTime();
  void CreateEventAndSend(const base::String& event);
  bool Tick(fml::TimePoint& time);
  void RequestNextFrame();
  void ResetPauseTiming();
  void InvalidateSampleCache();
  void ClearSampleHistory();
  AnimationDelegate* animation_delegate_{nullptr};
  base::String name_;
  // Process-unique identifier assigned once when the object is constructed.
  int64_t id_{0};
  std::unique_ptr<KeyframeEffect> keyframe_effect_;

  starlight::AnimationData animation_data_;

  tasm::Element* element_{nullptr};

  std::unordered_set<tasm::CSSPropertyID> raw_style_set_{};
  std::unordered_set<base::String> raw_custom_property_set_{};

  State state_{State::kIdle};

  bool is_transition_ = false;
  // Creation entry point exposed through the CDP Animation domain.
  Origin origin_{Origin::kCSSAnimation};
  bool need_report_over_time_{true};
  fml::TimePoint pause_time_{fml::TimePoint::Min()};
  fml::TimeDelta total_paused_duration_{fml::TimeDelta::Zero()};
  // Inspector timeline tracking. It uses the FML monotonic clock and does not
  // depend on the platform-specific vsync timestamp epoch.
  fml::TimeDelta current_time_at_pause_{fml::TimeDelta::Zero()};
  fml::TimePoint current_run_start_system_time_{fml::TimePoint::Min()};
  bool was_paused_{false};
  bool has_cached_sample_{false};
  fml::TimePoint cached_sample_time_{fml::TimePoint::Min()};
  KeyframeEffect::KeyframeSampleResult cached_sample_result_;
  bool has_last_sample_{false};
  fml::TimePoint last_sample_time_{fml::TimePoint::Min()};
  KeyframeEffect::KeyframeSampleResult last_sample_result_;
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_ANIMATION_H_
