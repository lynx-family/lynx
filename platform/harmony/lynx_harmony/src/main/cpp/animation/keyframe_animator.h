// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_KEYFRAME_ANIMATOR_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_KEYFRAME_ANIMATOR_H_

#include <arkui/native_animate.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/animation/animation_info.h"

namespace lynx {
namespace tasm {
namespace harmony {
class KeyframeAnimator;
class AnimationInfo;
class UIBase;

enum class AnimationProperty {
  kOpacity,
  kTranslateX,
  kTranslateY,
  kTranslateZ,
  kRotateX,
  kRotateY,
  kRotateZ,
  kScaleX,
  kScaleY,
};

enum class KFAnimatorState { kIdle, kRunning, kPaused, kCanceled };

struct Keyframe {
  Keyframe(float current, float v) : fraction(current), value(v) {}
  float fraction;
  float value;
};

struct KeyframeParsedData {
  std::unordered_map<AnimationProperty, std::vector<Keyframe>> keyframes_;

  bool has_transform_ = false;
  bool has_percentage_transform_ = false;

  std::set<AnimationProperty> has_start_keyframe_;
  std::set<AnimationProperty> has_end_keyframe_;
};

class ArkUIAnimation {
 public:
  ArkUIAnimation(KeyframeAnimator* animator, const AnimationInfo& info,
                 AnimationProperty type, const std::vector<Keyframe>& keyframes,
                 UIBase* ui);
  ~ArkUIAnimation();
  void Pause();
  void Run();
  void Cancel();
  void AddListenerToLastAnimator();

 private:
  struct UserData {
    KeyframeAnimator* animator;
    AnimationProperty type;
    UIBase* ui;
    std::string name;
  };
  UserData data_;
  ArkUI_AnimatorOption* option_;
  ArkUI_AnimatorHandle handle_ = nullptr;

  static ArkUI_NativeAnimateAPI_1* AnimationAPI();
};

class KeyframeAnimator {
 public:
  KeyframeAnimator(UIBase* ui);
  ~KeyframeAnimator();

  void Apply(const AnimationInfo& info);

  void Run(const std::string& name);
  void Pause();
  void Cancel();
  void Finish();

  bool IsRunning() const { return state_ == KFAnimatorState::kRunning; }

 private:
  void ApplyAnimationInfo(const AnimationInfo& info);
  bool ShouldReInitTransform(UIBase* ui) const;
  void ProcessKeyframes(std::vector<Keyframe>& vector,
                        const AnimationInfo& info, AnimationProperty type,
                        UIBase* ui);
  void AddListenerToLastAnimator();
  bool ParseKeyframes(UIBase* ui, const AnimationInfo& info);

  UIBase* ui_;
  AnimationInfo info_;
  KFAnimatorState state_ = KFAnimatorState::kIdle;

  std::unique_ptr<KeyframeParsedData> keyframe_parsed_data_;
  std::vector<std::unique_ptr<ArkUIAnimation>> animation_;
  float last_width_ = 0.f;
  float last_height_ = 0.f;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_KEYFRAME_ANIMATOR_H_
