// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_ANIMATION_INFO_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_ANIMATION_INFO_H_

#include <arkui/native_animate.h>

#include <string>

#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {
namespace harmony {
class AnimationInfo {
 public:
  static constexpr int PLAY_STATE_PAUSED = 0;
  static constexpr int PLAY_STATE_RUNNING = 1;

  AnimationInfo() = default;
  ~AnimationInfo() = default;

  bool IsEqualTo(const AnimationInfo& info) const;
  bool IsOnlyPlayStateChanged(const AnimationInfo& info) const;

  const std::string& GetName() const { return name_; }
  int64_t GetDuration() const { return duration_; }
  int64_t GetDelay() const { return delay_; }
  int GetTimingType() const { return timing_type_; }
  float GetX1() const { return x1_; }
  float GetY1() const { return y1_; }
  float GetX2() const { return x2_; }
  float GetY2() const { return y2_; }
  int GetStepsType() const { return steps_type_; }
  int GetIterationCount() const { return iteration_count_; }
  ArkUI_AnimationFillMode GetArkUIAnimationFillMode() const;
  ArkUI_AnimationDirection GetArkUIAnimationDirection() const;
  int GetPlayState() const { return play_state_; }

  void SetName(const std::string& name) { name_ = name; }
  void SetDuration(int64_t duration) { duration_ = duration; }
  void SetDelay(int64_t delay) { delay_ = delay; }
  void SetTimingType(int timing_type) { timing_type_ = timing_type; }
  void SetX1(float x1) { x1_ = x1; }
  void SetY1(float y1) { y1_ = y1; }
  void SetX2(float x2) { x2_ = x2; }
  void SetY2(float y2) { y2_ = y2; }
  void SetStepsType(int steps_type) { steps_type_ = steps_type; }
  void SetIterationCount(int iteration_count) {
    iteration_count_ = iteration_count;
  }
  void SetFillMode(int fill_mode) { fill_mode_ = fill_mode; }
  void SetDirection(int direction) { direction_ = direction; }
  void SetPlayState(int play_state) { play_state_ = play_state; }

  size_t SetTimingFunction(lepus::CArray* arr, size_t start);
  static AnimationInfo ToAnimationInfo(const lepus::Value& v);

 private:
  bool IsEqualExceptPlayState(const AnimationInfo& info) const;

  std::string name_;
  int64_t duration_{0};
  int64_t delay_{0};
  int timing_type_;
  float x1_{0.0f};
  float y1_{0.0f};
  float x2_{0.0f};
  float y2_{0.0f};
  int steps_type_;
  int iteration_count_{0};
  int fill_mode_;
  int direction_;
  int play_state_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_ANIMATION_INFO_H_
