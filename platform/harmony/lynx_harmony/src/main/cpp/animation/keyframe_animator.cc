// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/animation/keyframe_animator.h"

#include <arkui/native_animate.h>
#include <arkui/native_interface.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

#include "base/include/algorithm.h"
#include "base/include/string/string_number_convert.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/animation/animation_info.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

ArkUIAnimation::ArkUIAnimation(KeyframeAnimator* animator,
                               const AnimationInfo& info,
                               AnimationProperty type,
                               const std::vector<Keyframe>& keyframes,
                               UIBase* ui)
    : data_({animator, type, ui, info.GetName()}) {
  option_ = OH_ArkUI_AnimatorOption_Create(keyframes.size());
  OH_ArkUI_AnimatorOption_SetDuration(option_, info.GetDuration());
  OH_ArkUI_AnimatorOption_SetDelay(option_, info.GetDelay());
  OH_ArkUI_AnimatorOption_SetIterations(option_, info.GetIterationCount());
  OH_ArkUI_AnimatorOption_SetFill(option_, info.GetArkUIAnimationFillMode());
  OH_ArkUI_AnimatorOption_SetDirection(option_,
                                       info.GetArkUIAnimationDirection());
  ArkUI_CurveHandle curve;

  switch (static_cast<starlight::TimingFunctionType>(info.GetTimingType())) {
    case starlight::TimingFunctionType::kLinear:
      curve = OH_ArkUI_Curve_CreateCurveByType(ARKUI_CURVE_LINEAR);
      break;
    case starlight::TimingFunctionType::kEaseIn:
      curve = OH_ArkUI_Curve_CreateCurveByType(ARKUI_CURVE_EASE_IN);
      break;
    case starlight::TimingFunctionType::kEaseOut:
      curve = OH_ArkUI_Curve_CreateCurveByType(ARKUI_CURVE_EASE_OUT);
      break;
    case starlight::TimingFunctionType::kEaseInEaseOut:
      curve = OH_ArkUI_Curve_CreateCurveByType(ARKUI_CURVE_EASE_IN_OUT);
      break;
    case starlight::TimingFunctionType::kSquareBezier:
      curve = OH_ArkUI_Curve_CreateCubicBezierCurve(info.GetX1(), info.GetY1(),
                                                    0, 0);
      break;
    case starlight::TimingFunctionType::kCubicBezier:
      curve = OH_ArkUI_Curve_CreateCubicBezierCurve(info.GetX1(), info.GetY1(),
                                                    info.GetX2(), info.GetY2());
      break;
    case starlight::TimingFunctionType::kSteps:
      curve =
          OH_ArkUI_Curve_CreateStepsCurve(info.GetX1(), info.GetStepsType());
      break;
    default:
      curve = OH_ArkUI_Curve_CreateCurveByType(ARKUI_CURVE_LINEAR);
  }
  OH_ArkUI_AnimatorOption_SetCurve(option_, curve);

  for (size_t i = 0; i < keyframes.size(); i++) {
    OH_ArkUI_AnimatorOption_SetKeyframe(option_, keyframes[i].fraction,
                                        keyframes[i].value, i);
  }

  OH_ArkUI_AnimatorOption_RegisterOnFrameCallback(
      option_, &data_, [](ArkUI_AnimatorOnFrameEvent* event) {
        auto* data = static_cast<UserData*>(
            OH_ArkUI_AnimatorOnFrameEvent_GetUserData(event));
        auto value = OH_ArkUI_AnimatorOnFrameEvent_GetValue(event);
        data->ui->SetAnimationProperty(data->type, value);
      });
}

ArkUIAnimation::~ArkUIAnimation() {
  OH_ArkUI_AnimatorOption_Dispose(option_);
  if (handle_) {
    AnimationAPI()->disposeAnimator(handle_);
  }
}

void ArkUIAnimation::Pause() {
  if (handle_) {
    OH_ArkUI_Animator_Pause(handle_);
  }
}

void ArkUIAnimation::Run() {
  if (!handle_) {
    handle_ = AnimationAPI()->createAnimator(
        data_.ui->GetContext()->ArkUIContext(), option_);
  }
  OH_ArkUI_Animator_Play(handle_);
}

void ArkUIAnimation::Cancel() {
  if (handle_) {
    OH_ArkUI_Animator_Cancel(handle_);
  }
}

void ArkUIAnimation::AddListenerToLastAnimator() {
  OH_ArkUI_AnimatorOption_RegisterOnFinishCallback(
      option_, &data_, [](ArkUI_AnimatorEvent* event) {
        auto* data =
            static_cast<UserData*>(OH_ArkUI_AnimatorEvent_GetUserData(event));
        data->ui->SendAnimationEvent("animationend", data->name);
        data->animator->Finish();
      });

  OH_ArkUI_AnimatorOption_RegisterOnCancelCallback(
      option_, &data_, [](ArkUI_AnimatorEvent* event) {
        auto* data =
            static_cast<UserData*>(OH_ArkUI_AnimatorEvent_GetUserData(event));
        data->ui->SendAnimationEvent("animationcancel", data->name);
      });

  OH_ArkUI_AnimatorOption_RegisterOnRepeatCallback(
      option_, &data_, [](ArkUI_AnimatorEvent* event) {
        auto* data =
            static_cast<UserData*>(OH_ArkUI_AnimatorEvent_GetUserData(event));
        data->ui->SendAnimationEvent("animationiteration", data->name);
      });
}

ArkUI_NativeAnimateAPI_1* ArkUIAnimation::AnimationAPI() {
  static ArkUI_NativeAnimateAPI_1* api = nullptr;
  OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_ANIMATE, ArkUI_NativeAnimateAPI_1,
                              api);
  return api;
}

KeyframeAnimator::KeyframeAnimator(UIBase* ui) : ui_(ui) {}

KeyframeAnimator::~KeyframeAnimator() { Cancel(); }

bool KeyframeAnimator::ShouldReInitTransform(UIBase* ui) const {
  if (!ui) {
    return false;
  }
  if (!keyframe_parsed_data_ ||
      !keyframe_parsed_data_->has_percentage_transform_) {
    return false;
  }
  return ui->width_ != last_width_ || ui->height_ != last_height_;
}

void KeyframeAnimator::Apply(const AnimationInfo& info) {
  if (!ui_) return;

  switch (state_) {
    case KFAnimatorState::kIdle:
    case KFAnimatorState::kCanceled: {
      if (info.GetIterationCount() < 0 || info.GetDuration() <= 0) return;
      ApplyAnimationInfo(info);
      break;
    }
    case KFAnimatorState::kPaused:
    case KFAnimatorState::kRunning: {
      if (info.IsEqualTo(info_) && !ShouldReInitTransform(ui_)) {
        return;
      }
      if (info.IsOnlyPlayStateChanged(info_)) {
        if (state_ == KFAnimatorState::kPaused) {
          Run(info.GetName());
        } else {
          Pause();
        }
      } else {
        Cancel();
        Apply(info);
      }
      break;
    }
    default:
      break;
  }
  info_ = info;
}

void KeyframeAnimator::ApplyAnimationInfo(const AnimationInfo& info) {
  if (!ui_) return;
  if (!ParseKeyframes(ui_, info)) {
    return;
  }
  animation_.clear();
  for (const auto& [type, keyframes] : keyframe_parsed_data_->keyframes_) {
    animation_.emplace_back(
        std::make_unique<ArkUIAnimation>(this, info, type, keyframes, ui_));
  }
  AddListenerToLastAnimator();
  if (state_ == KFAnimatorState::kIdle) {
    ui_->SendAnimationEvent("animationstart", info.GetName());
  }

  Run(info.GetName());
  if (info.GetPlayState() == AnimationInfo::PLAY_STATE_PAUSED) {
    Pause();
  }
}

void KeyframeAnimator::ProcessKeyframes(std::vector<Keyframe>& vector,
                                        const AnimationInfo& info,
                                        AnimationProperty type, UIBase* ui) {
  if (vector.empty()) {
    return;
  }
  if (keyframe_parsed_data_->has_start_keyframe_.count(type) == 0) {
    vector.emplace_back(0, ui->GetArkUIProperty(type));
  }
  if (keyframe_parsed_data_->has_end_keyframe_.count(type) == 0) {
    vector.emplace_back(1, ui->GetArkUIProperty(type));
  }
  base::InsertionSort(vector.data(), vector.size(),
                      [](const Keyframe& a, const Keyframe& b) {
                        return a.fraction < b.fraction;
                      });
}

void KeyframeAnimator::AddListenerToLastAnimator() {
  if (animation_.empty()) return;
  const auto& animation = animation_[animation_.size() - 1];
  animation->AddListenerToLastAnimator();
}

void KeyframeAnimator::Run(const std::string& name) {
  state_ = KFAnimatorState::kRunning;
  for (const auto& animation : animation_) {
    animation->Run();
  }
}

void KeyframeAnimator::Pause() {
  state_ = KFAnimatorState::kPaused;
  for (const auto& animation : animation_) {
    animation->Pause();
  }
}

void KeyframeAnimator::Cancel() {
  state_ = KFAnimatorState::kCanceled;
  for (const auto& animation : animation_) {
    animation->Cancel();
  }
}

void KeyframeAnimator::Finish() { state_ = KFAnimatorState::kIdle; }

static void CalStartEnd(float time, AnimationProperty type,
                        KeyframeParsedData* keyframe_parsed_data) {
  if (0 == time) {
    keyframe_parsed_data->has_start_keyframe_.insert(type);
  }
  if (1 == time) {
    keyframe_parsed_data->has_end_keyframe_.insert(type);
  }
}

static void MarkRelative(KeyframeParsedData* keyframe_parsed_data,
                         const PlatformLength& p) {
  if (p.IsRelative()) {
    keyframe_parsed_data->has_percentage_transform_ = true;
  }
};

bool KeyframeAnimator::ParseKeyframes(UIBase* ui, const AnimationInfo& info) {
  if (keyframe_parsed_data_ && !ShouldReInitTransform(ui)) {
    return true;
  }
  if (ShouldReInitTransform(ui)) {
    keyframe_parsed_data_.reset();
  }
  keyframe_parsed_data_ = std::make_unique<KeyframeParsedData>();
  last_width_ = ui->width_;
  last_height_ = ui->height_;
  const auto& keyframe_map = ui->GetKeyframes(info.GetName());
  if (!keyframe_map.IsTable()) {
    return false;
  }
  for (const auto& [time, value] : *keyframe_map.Table()) {
    float current = 0;
    base::StringToFloat(time.str(), current);

    for (const auto& [property, style] : *value.Table()) {
      if (property == "opacity") {
        CalStartEnd(current, AnimationProperty::kOpacity,
                    keyframe_parsed_data_.get());
        float styleValue = style.Number();
        if (styleValue < 0 || styleValue > 1) {
          return false;
        }
        keyframe_parsed_data_->keyframes_[AnimationProperty::kOpacity]
            .emplace_back(current, styleValue);
      } else if (property == "transform") {
        auto transform = std::make_unique<Transform>(style);
        keyframe_parsed_data_->has_transform_ = true;
        const auto& transform_raw = transform->GetTransformRaw();
        for (const auto& t : transform_raw) {
          switch (t.func_type) {
            case starlight::TransformType::kTranslateX:
              MarkRelative(keyframe_parsed_data_.get(), t.params[0]);
              CalStartEnd(current, AnimationProperty::kTranslateX,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kTranslateX]
                  .emplace_back(current, t.params[0].GetValue(ui->width_));
              break;
            case starlight::TransformType::kTranslateY:
              MarkRelative(keyframe_parsed_data_.get(), t.params[0]);
              CalStartEnd(current, AnimationProperty::kTranslateY,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kTranslateY]
                  .emplace_back(current, t.params[0].GetValue(ui->height_));
              break;
            case starlight::TransformType::kTranslate:
              MarkRelative(keyframe_parsed_data_.get(), t.params[0]);
              MarkRelative(keyframe_parsed_data_.get(), t.params[1]);
              CalStartEnd(current, AnimationProperty::kTranslateX,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kTranslateX]
                  .emplace_back(current, t.params[0].GetValue(ui->width_));
              CalStartEnd(current, AnimationProperty::kTranslateY,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kTranslateY]
                  .emplace_back(current, t.params[1].GetValue(ui->height_));
              break;
            case starlight::TransformType::kTranslateZ:
              CalStartEnd(current, AnimationProperty::kTranslateZ,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kTranslateZ]
                  .emplace_back(current, t.params[0].GetValue(0));
              break;
            case starlight::TransformType::kRotate:
            case starlight::TransformType::kRotateZ:
              CalStartEnd(current, AnimationProperty::kRotateZ,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kRotateZ]
                  .emplace_back(current, t.params[0].GetValue(0));
              break;
            case starlight::TransformType::kRotateX:
              CalStartEnd(current, AnimationProperty::kRotateX,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kRotateX]
                  .emplace_back(current, t.params[0].GetValue(0));
              break;
            case starlight::TransformType::kRotateY:
              CalStartEnd(current, AnimationProperty::kRotateY,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kRotateY]
                  .emplace_back(current, t.params[0].GetValue(0));
              break;
            case starlight::TransformType::kScaleX:
              CalStartEnd(current, AnimationProperty::kScaleX,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kScaleX]
                  .emplace_back(current, t.params[0].GetValue(0));
              break;
            case starlight::TransformType::kScaleY:
              CalStartEnd(current, AnimationProperty::kScaleY,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kScaleY]
                  .emplace_back(current, t.params[0].GetValue(0));
              break;
            case starlight::TransformType::kScale:
              CalStartEnd(current, AnimationProperty::kScaleX,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kScaleX]
                  .emplace_back(current, t.params[0].GetValue(0));
              CalStartEnd(current, AnimationProperty::kScaleY,
                          keyframe_parsed_data_.get());
              keyframe_parsed_data_->keyframes_[AnimationProperty::kScaleY]
                  .emplace_back(current, t.params[1].GetValue(0));
              break;
            default:
              break;
          }
        }
      }
    }
  }

  for (auto& [type, keyframes] : keyframe_parsed_data_->keyframes_) {
    ProcessKeyframes(keyframes, info, type, ui);
  }
  return true;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
