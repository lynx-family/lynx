// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATION_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATION_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/basic_animation/animation_effect.h"
#include "core/animation/basic_animation/animation_frame_callback.h"
#include "core/animation/basic_animation/animation_timeline.h"
#include "core/animation/basic_animation/basic_keyframe_effect.h"

namespace lynx {
namespace animation {
namespace basic {

class AnimationFrameCallbackProvider;
class AnimationEventListener;

class Animation : public AnimationFrameCallback {
 public:
  enum class State { kIdle = 0, kPlay, kPause, kStop };
  enum PlayState { PAUSED, RUNNING };
  enum EventType { Start, End, Cancel, Iteration };

  const std::string& animation_name() const { return data_.animation_name(); }

  fml::TimeDelta current_time() const { return data_.current_time(); }

  double playback_rate() const { return data_.playback_rate(); }

  PlayState play_state() const { return data_.play_state(); }

  void set_play_state(PlayState play_state) {
    data_.set_play_state(play_state);
  }

 public:
  class Data {
   public:
    Data() = default;
    Data(const std::string& animation_name, const fml::TimeDelta& start_time,
         const fml::TimeDelta& current_time, PlayState play_state)
        : animation_name_(animation_name),
          current_time_(current_time),
          play_state_(play_state) {}

    const std::string& animation_name() const { return animation_name_; }

    fml::TimeDelta current_time() const { return current_time_; }

    double playback_rate() const { return playback_rate_; }

    PlayState play_state() const { return play_state_; }

    void set_play_state(PlayState play_state) { play_state_ = play_state; }

   private:
    std::string animation_name_;
    fml::TimeDelta current_time_;
    double playback_rate_ = 1.0;
    PlayState play_state_;
  };

 public:
  Animation(std::unique_ptr<KeyframeEffect> effect,
            std::unique_ptr<AnimationTimeLine> timeline)
      : effect_(std::move(effect)), timeline_(std::move(timeline)) {
    effect_->BindHostAnimation(this);
  };

  explicit Animation(std::unique_ptr<KeyframeEffect> effect)
      : effect_(std::move(effect)) {
    effect_->BindHostAnimation(this);
  };

  void AddEventListener(
      const std::shared_ptr<AnimationEventListener>& listener) {
    listener_ = std::weak_ptr<AnimationEventListener>(listener);
  }

  void RegisterAnimationFrameCallbackProvider(
      const std::weak_ptr<AnimationFrameCallbackProvider>& provider) {
    animation_frame_callback_provider_ = provider;
  }

 public:
  void Play();
  void Pause();
  void Stop();
  void Destroy(bool need_clear_effect = true);
  void DoAnimationFrame(const fml::TimePoint& frame_time) override;
  void Tick(const fml::TimePoint& time);
  State GetState() { return state_; }

  const AnimationEffect& GetEffect() { return *effect_; }

  static fml::TimePoint& GetAnimationDummyStartTime();

  void RequestNextFrame();

  bool HasFinishAll(const fml::TimePoint& time);

  void SendAnimationEvent(EventType type);

 protected:
  Data data_;
  fml::TimePoint start_time_{fml::TimePoint::Min()};

 private:
  State state_{State::kIdle};
  std::unique_ptr<AnimationEffect> effect_;
  std::unique_ptr<AnimationTimeLine> timeline_;
  std::weak_ptr<AnimationEventListener> listener_;
  std::weak_ptr<AnimationFrameCallbackProvider>
      animation_frame_callback_provider_;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATION_H_
