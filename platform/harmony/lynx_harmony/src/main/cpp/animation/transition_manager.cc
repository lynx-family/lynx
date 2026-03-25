// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/animation/transition_manager.h"

#include <arkui/native_interface.h>
#include <arkui/native_node.h>

#include <algorithm>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "core/animation/basic_animation/basic_animatable_values/color_property_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/transform.h"

namespace lynx {
namespace tasm {
namespace harmony {

using AnimationPropertyType = starlight::AnimationPropertyType;
static AnimationPropertyType kAllTransitionProperties[] = {
    AnimationPropertyType::kOpacity,
    AnimationPropertyType::kWidth,
    AnimationPropertyType::kHeight,
    AnimationPropertyType::kLeft,
    AnimationPropertyType::kTop,
    AnimationPropertyType::kRight,
    AnimationPropertyType::kBottom,
    AnimationPropertyType::kTransform,
    AnimationPropertyType::kBackgroundColor};

namespace {

ArkUI_NativeAnimateAPI_1* AnimationAPI() {
  static ArkUI_NativeAnimateAPI_1* api = nullptr;
  if (!api) {
    OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_ANIMATE, ArkUI_NativeAnimateAPI_1,
                                api);
  }
  return api;
}

const char* TransitionPropertyName(AnimationPropertyType property) {
  switch (property) {
    case AnimationPropertyType::kOpacity:
      return "opacity";
    case AnimationPropertyType::kWidth:
      return "width";
    case AnimationPropertyType::kHeight:
      return "height";
    case AnimationPropertyType::kLeft:
      return "left";
    case AnimationPropertyType::kTop:
      return "top";
    case AnimationPropertyType::kRight:
      return "right";
    case AnimationPropertyType::kBottom:
      return "bottom";
    case AnimationPropertyType::kTransform:
      return "transform";
    case AnimationPropertyType::kBackgroundColor:
      return "background-color";
    default:
      return "none";
  }
}

}  // namespace

TransitionManager::TransitionManager(UIBase* ui) : ui_(ui) {}

class TransitionAnimator {
 public:
  TransitionAnimator(UIBase* ui, AnimationPropertyType property,
                     const AnimationInfo& info, float start, float end,
                     bool send_event)
      : ui_(ui),
        property_(property),
        info_(info),
        start_(start),
        end_(end),
        send_event_(send_event) {
    option_ = OH_ArkUI_AnimatorOption_Create(0);
    OH_ArkUI_AnimatorOption_SetDuration(option_, info.GetDuration());
    OH_ArkUI_AnimatorOption_SetDelay(option_, info.GetDelay());
    OH_ArkUI_AnimatorOption_SetIterations(option_, 1);
    OH_ArkUI_AnimatorOption_SetFill(option_, ARKUI_ANIMATION_FILL_MODE_BOTH);
    OH_ArkUI_AnimatorOption_SetBegin(option_, start);
    OH_ArkUI_AnimatorOption_SetEnd(option_, end);

    ArkUI_CurveHandle curve = nullptr;
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
        curve = OH_ArkUI_Curve_CreateCubicBezierCurve(info.GetX1(),
                                                      info.GetY1(), 1, 1);
        break;
      case starlight::TimingFunctionType::kCubicBezier:
        curve = OH_ArkUI_Curve_CreateCubicBezierCurve(
            info.GetX1(), info.GetY1(), info.GetX2(), info.GetY2());
        break;
      case starlight::TimingFunctionType::kSteps:
        curve =
            OH_ArkUI_Curve_CreateStepsCurve(info.GetX1(), info.GetStepsType());
        break;
      default:
        curve = OH_ArkUI_Curve_CreateCurveByType(ARKUI_CURVE_LINEAR);
    }
    OH_ArkUI_AnimatorOption_SetCurve(option_, curve);

    OH_ArkUI_AnimatorOption_RegisterOnFrameCallback(
        option_, this, [](ArkUI_AnimatorOnFrameEvent* event) {
          auto* animator = static_cast<TransitionAnimator*>(
              OH_ArkUI_AnimatorOnFrameEvent_GetUserData(event));
          float fraction = OH_ArkUI_AnimatorOnFrameEvent_GetValue(event);
          animator->OnFrame(fraction);
        });

    OH_ArkUI_AnimatorOption_RegisterOnFinishCallback(
        option_, this, [](ArkUI_AnimatorEvent* event) {
          auto* animator = static_cast<TransitionAnimator*>(
              OH_ArkUI_AnimatorEvent_GetUserData(event));
          animator->OnFinish();
        });

    OH_ArkUI_AnimatorOption_RegisterOnCancelCallback(
        option_, this, [](ArkUI_AnimatorEvent* event) {
          auto* animator = static_cast<TransitionAnimator*>(
              OH_ArkUI_AnimatorEvent_GetUserData(event));
          animator->OnCancel();
        });
  }

  virtual ~TransitionAnimator() {
    if (handle_) {
      AnimationAPI()->disposeAnimator(handle_);
    }
    OH_ArkUI_AnimatorOption_Dispose(option_);
  }

  void Start() {
    if (!handle_) {
      handle_ = AnimationAPI()->createAnimator(
          ui_->GetContext()->ArkUIContext(), option_);
    }
    OH_ArkUI_Animator_Play(handle_);
    if (send_event_) {
      ui_->SendAnimationEvent("transitionstart",
                              TransitionPropertyName(property_));
    }
  }

  void Cancel() {
    if (handle_) {
      OH_ArkUI_Animator_Cancel(handle_);
    }
  }

  bool IsFinished() const { return finished_; }

  virtual void OnFrame(float fraction) = 0;

  virtual void OnFinish() {
    if (finished_) {
      return;
    }
    if (send_event_) {
      ui_->SendAnimationEvent("transitionend",
                              TransitionPropertyName(property_));
    }
    finished_ = true;
  }

  virtual void OnCancel() { finished_ = true; }

 protected:
  UIBase* ui_;
  AnimationPropertyType property_;
  AnimationInfo info_;
  ArkUI_AnimatorOption* option_;
  ArkUI_AnimatorHandle handle_ = nullptr;
  float start_{0.f};
  float end_{0.f};
  bool send_event_{true};
  bool finished_{false};
};

class OpacityTransitionAnimator : public TransitionAnimator {
 public:
  OpacityTransitionAnimator(UIBase* ui, AnimationPropertyType property,
                            const AnimationInfo& info, float start, float end)
      : TransitionAnimator(ui, property, info, start, end, true) {}

  void OnFrame(float val) override {
    ui_->SetAnimationProperty(AnimationProperty::kOpacity, val);
  }

  void OnFinish() override {
    ui_->SetAnimationProperty(AnimationProperty::kOpacity, end_);
    TransitionAnimator::OnFinish();
  }
};

class TransformTransitionAnimator : public TransitionAnimator {
 public:
  TransformTransitionAnimator(UIBase* ui, AnimationPropertyType property,
                              const AnimationInfo& info,
                              std::unique_ptr<Transform> transform)
      : TransitionAnimator(ui, property, info, 0, 1, true) {
    start_translate_x_ = ui->GetArkUIProperty(AnimationProperty::kTranslateX);
    start_translate_y_ = ui->GetArkUIProperty(AnimationProperty::kTranslateY);
    start_translate_z_ = ui->GetArkUIProperty(AnimationProperty::kTranslateZ);
    start_rotation_x_ = ui->GetArkUIProperty(AnimationProperty::kRotateX);
    start_rotation_y_ = ui->GetArkUIProperty(AnimationProperty::kRotateY);
    start_rotation_z_ = ui->GetArkUIProperty(AnimationProperty::kRotateZ);
    start_scale_x_ = ui->GetArkUIProperty(AnimationProperty::kScaleX);
    start_scale_y_ = ui->GetArkUIProperty(AnimationProperty::kScaleY);

    const auto& transform_raw = transform->GetTransformRaw();
    for (const auto& t : transform_raw) {
      switch (t.func_type) {
        case starlight::TransformType::kTranslateX:
          end_translate_x_ = t.params[0].GetValue(ui->width_);
          break;
        case starlight::TransformType::kTranslateY:
          end_translate_y_ = t.params[0].GetValue(ui->height_);
          break;
        case starlight::TransformType::kTranslate:
          end_translate_x_ = t.params[0].GetValue(ui->width_);
          end_translate_y_ = t.params[1].GetValue(ui->height_);
          break;
        case starlight::TransformType::kTranslateZ:
          end_translate_z_ = t.params[0].GetValue(0);
          break;
        case starlight::TransformType::kRotate:
        case starlight::TransformType::kRotateZ:
          end_rotation_z_ = t.params[0].GetValue(0);
          break;
        case starlight::TransformType::kRotateX:
          end_rotation_x_ = t.params[0].GetValue(0);
          break;
        case starlight::TransformType::kRotateY:
          end_rotation_y_ = t.params[0].GetValue(0);
          break;
        case starlight::TransformType::kScaleX:
          end_scale_x_ = t.params[0].GetValue(0);
          break;
        case starlight::TransformType::kScaleY:
          end_scale_y_ = t.params[0].GetValue(0);
          break;
        case starlight::TransformType::kScale:
          end_scale_x_ = t.params[0].GetValue(0);
          end_scale_y_ = t.params[1].GetValue(0);
          break;
        default:
          break;
      }
    }
  }

  float NewValue(float start, float end, float fraction) {
    return start + (end - start) * fraction;
  }

  void OnFrame(float fraction) override {
    auto trans_x = NewValue(start_translate_x_, end_translate_x_, fraction);
    auto trans_y = NewValue(start_translate_y_, end_translate_y_, fraction);
    auto trans_z = NewValue(start_translate_z_, end_translate_z_, fraction);
    auto rota_x = NewValue(start_rotation_x_, end_rotation_x_, fraction);
    auto rota_y = NewValue(start_rotation_y_, end_rotation_y_, fraction);
    auto rota_z = NewValue(start_rotation_z_, end_rotation_z_, fraction);
    auto scale_x = NewValue(start_scale_x_, end_scale_x_, fraction);
    auto scale_y = NewValue(start_scale_y_, end_scale_y_, fraction);
    ui_->SetTransformProperty(trans_x, trans_y, trans_z, rota_x, rota_y, rota_z,
                              scale_x, scale_y);
  }

  void OnFinish() override {
    ui_->FinishTransformTransition();
    TransitionAnimator::OnFinish();
  }

 private:
  float start_translate_x_{0.f};
  float start_translate_y_{0.f};
  float start_translate_z_{0.f};
  float start_rotation_x_{0.f};
  float start_rotation_y_{0.f};
  float start_rotation_z_{0.f};
  float start_scale_x_{1.f};
  float start_scale_y_{1.f};
  float end_translate_x_{0.f};
  float end_translate_y_{0.f};
  float end_translate_z_{0.f};
  float end_rotation_x_{0.f};
  float end_rotation_y_{0.f};
  float end_rotation_z_{0.f};
  float end_scale_x_{1.f};
  float end_scale_y_{1.f};
};

class BackgroundColorTransitionAnimator : public TransitionAnimator {
 public:
  BackgroundColorTransitionAnimator(UIBase* ui, AnimationPropertyType property,
                                    const AnimationInfo& info, uint32_t start,
                                    uint32_t end)
      : TransitionAnimator(ui, property, info, 0.f, 1.f, true),
        end_color_(end),
        start_value_(start),
        end_value_(
            std::make_unique<lynx::animation::basic::ColorPropertyValue>(end)) {
  }

  void OnFrame(float fraction) override {
    auto result = start_value_.Interpolate(fraction, end_value_);
    auto* color_value =
        static_cast<animation::basic::ColorPropertyValue*>(result.get());
    ui_->UpdateBackgroundColor(color_value->GetColorValue());
  }

  void OnFinish() override {
    ui_->UpdateBackgroundColor(end_color_);
    TransitionAnimator::OnFinish();
  }

 private:
  uint32_t end_color_{0};
  animation::basic::ColorPropertyValue start_value_;
  std::unique_ptr<animation::basic::PropertyValue> end_value_;
};

class LayoutTransitionAnimator : public TransitionAnimator {
 public:
  LayoutTransitionAnimator(UIBase* ui, AnimationPropertyType property,
                           const AnimationInfo& info, float start, float end,
                           bool send_event)
      : TransitionAnimator(ui, property, info, start, end, send_event) {}

  void OnFrame(float val) override {
    switch (property_) {
      case AnimationPropertyType::kLeft:
      case AnimationPropertyType::kRight:
        ui_->left_ = val;
        break;
      case AnimationPropertyType::kTop:
      case AnimationPropertyType::kBottom:
        ui_->top_ = val;
        break;
      case AnimationPropertyType::kWidth:
        ui_->width_ = val;
        break;
      case AnimationPropertyType::kHeight:
        ui_->height_ = val;
        break;
      default:
        break;
    }
    ui_->FrameDidChanged();
  }

  void OnFinish() override {
    switch (property_) {
      case AnimationPropertyType::kLeft:
      case AnimationPropertyType::kRight:
        ui_->left_ = end_;
        break;
      case AnimationPropertyType::kTop:
      case AnimationPropertyType::kBottom:
        ui_->top_ = end_;
        break;
      case AnimationPropertyType::kWidth:
        ui_->width_ = end_;
        break;
      case AnimationPropertyType::kHeight:
        ui_->height_ = end_;
        break;
      default:
        break;
    }
    ui_->FrameDidChanged();
    TransitionAnimator::OnFinish();
  }
};

TransitionManager::~TransitionManager() = default;

bool TransitionManager::InitializeFromConfig(const lepus::Value& config) {
  if (config.IsNil() || !config.IsArrayOrJSArray()) {
    for (auto& [prop, animator] : running_animators_) {
      if (animator) {
        animator->Cancel();
      }
    }
    running_animators_.clear();
    for (auto& [prop, animator] : idle_animators_) {
      if (animator) {
        animator->Cancel();
      }
    }
    idle_animators_.clear();
    animation_infos_.clear();
    return false;
  }
  auto old_animations = std::move(animation_infos_);
  auto arr = config.Array();
  for (size_t i = 0; i < arr->size(); ++i) {
    const auto& item = arr->get(i);
    if (!item.IsArrayOrJSArray()) continue;
    const auto& prop_config = item.Array();
    if (prop_config->size() < 2) continue;

    auto property =
        static_cast<AnimationPropertyType>(prop_config->get(0).Number());
    AnimationInfo info;
    info.SetDuration(static_cast<int64_t>(prop_config->get(1).Number()));
    size_t next = info.SetTimingFunction(prop_config.get(), 2);
    if (next < prop_config->size()) {
      info.SetDelay(static_cast<int64_t>(prop_config->get(next).Number()));
    }
    info.SetOrderIndex(i);
    if (property == AnimationPropertyType::kAll ||
        property == AnimationPropertyType::kLegacyAll_1 ||
        property == AnimationPropertyType::kLegacyAll_2 ||
        property == AnimationPropertyType::kLegacyAll_3) {
      animation_infos_.clear();
      int index = 0;
      for (auto value : kAllTransitionProperties) {
        AnimationInfo ai = info;
        ai.SetOrderIndex(index++);
        animation_infos_[value] = ai;
      }
      break;
    }
    animation_infos_[property] = info;
  }
  RemoveDuplicateAnimation(AnimationPropertyType::kLeft,
                           AnimationPropertyType::kRight);
  RemoveDuplicateAnimation(AnimationPropertyType::kTop,
                           AnimationPropertyType::kBottom);

  // cancel animations that were removed from css
  for (const auto& [prop, info] : old_animations) {
    if (animation_infos_.find(prop) == animation_infos_.end()) {
      EndTransitionAnimator(prop);
    }
  }
  return !animation_infos_.empty();
}

void TransitionManager::ApplyPropertyTransition(AnimationPropertyType property,
                                                const lepus::Value& value) {
  if (animation_infos_.find(property) == animation_infos_.end()) {
    return;
  }
  const auto& info = animation_infos_[property];
  std::unique_ptr<TransitionAnimator> animator;

  if (property == AnimationPropertyType::kOpacity) {
    float start = ui_->GetArkUIProperty(AnimationProperty::kOpacity);
    float end = value.IsNumber() ? value.Number() : 1.0f;
    animator = std::make_unique<OpacityTransitionAnimator>(ui_, property, info,
                                                           start, end);
  } else if (property == AnimationPropertyType::kTransform) {
    animator = std::make_unique<TransformTransitionAnimator>(
        ui_, property, info, std::make_unique<Transform>(value));
  } else if (property == AnimationPropertyType::kBackgroundColor) {
    uint32_t start = ui_->GetBackgroundColor();
    uint32_t end = value.IsNil() ? 0 : static_cast<uint32_t>(value.Number());
    animator = std::make_unique<BackgroundColorTransitionAnimator>(
        ui_, property, info, start, end);
  }

  if (animator) {
    idle_animators_[property] = std::move(animator);
  }
}

void TransitionManager::ApplyLayoutTransition(
    float left, float top, float width, float height, const float* paddings,
    const float* margins, const float* sticky, float max_height,
    uint32_t node_index) {
  bool x_pos_change = base::FloatsNotEqual(ui_->left_, left);
  bool y_pos_change = base::FloatsNotEqual(ui_->top_, top);
  bool width_change = base::FloatsNotEqual(ui_->width_, width);
  bool height_change = base::FloatsNotEqual(ui_->height_, height);
  bool x_right_not_change =
      base::FloatsEqual(ui_->left_ + ui_->width_, left + width);
  bool y_bottom_not_change =
      base::FloatsEqual(ui_->top_ + ui_->height_, top + height);

  bool has_left = animation_infos_.count(AnimationPropertyType::kLeft) ||
                  animation_infos_.count(AnimationPropertyType::kRight);
  bool has_top = animation_infos_.count(AnimationPropertyType::kTop) ||
                 animation_infos_.count(AnimationPropertyType::kBottom);
  bool has_width = animation_infos_.count(AnimationPropertyType::kWidth);
  bool has_height = animation_infos_.count(AnimationPropertyType::kHeight);

  float start_x = left;
  if ((has_left && x_pos_change) || (has_width && x_right_not_change)) {
    start_x = ui_->left_;
  }
  float start_y = top;
  if ((has_top && y_pos_change) || (has_height && y_bottom_not_change)) {
    start_y = ui_->top_;
  }
  float start_width = width_change && has_width ? ui_->width_ : width;
  float start_height = height_change && has_height ? ui_->height_ : height;

  EndTransitionAnimator(AnimationPropertyType::kLeft);
  EndTransitionAnimator(AnimationPropertyType::kRight);
  EndTransitionAnimator(AnimationPropertyType::kTop);
  EndTransitionAnimator(AnimationPropertyType::kBottom);
  EndTransitionAnimator(AnimationPropertyType::kWidth);
  EndTransitionAnimator(AnimationPropertyType::kHeight);

  ui_->UpdateLayout(start_x, start_y, start_width, start_height, paddings,
                    margins, sticky, max_height, node_index);

  auto create_layout_animator = [this](AnimationPropertyType prop,
                                       const AnimationInfo& info, float start,
                                       float end, bool send_event) {
    auto animator = std::make_unique<LayoutTransitionAnimator>(
        ui_, prop, info, start, end, send_event);
    idle_animators_[prop] = std::move(animator);
  };

  for (const auto& [property, info] : animation_infos_) {
    if ((property == AnimationPropertyType::kLeft ||
         property == AnimationPropertyType::kRight) &&
        x_pos_change) {
      create_layout_animator(property, info, ui_->left_, left, true);
    } else if ((property == AnimationPropertyType::kTop ||
                property == AnimationPropertyType::kBottom) &&
               y_pos_change) {
      create_layout_animator(property, info, ui_->top_, top, true);
    } else if (property == AnimationPropertyType::kWidth && width_change) {
      create_layout_animator(AnimationPropertyType::kWidth, info, ui_->width_,
                             width, true);
      bool need_additional_animator =
          x_pos_change && x_right_not_change && !has_left;
      if (need_additional_animator) {
        create_layout_animator(AnimationPropertyType::kLeft, info, ui_->left_,
                               left, false);
      }
    } else if (property == AnimationPropertyType::kHeight && height_change) {
      create_layout_animator(AnimationPropertyType::kHeight, info, ui_->height_,
                             height, true);
      bool need_additional_animator =
          y_pos_change && y_bottom_not_change && !has_top;
      if (need_additional_animator) {
        create_layout_animator(AnimationPropertyType::kTop, info, ui_->top_,
                               top, false);
      }
    }
  }
}

bool TransitionManager::HasLayoutTransition() {
  return !animation_infos_.empty() &&
         (animation_infos_.count(AnimationPropertyType::kLeft) ||
          animation_infos_.count(AnimationPropertyType::kRight) ||
          animation_infos_.count(AnimationPropertyType::kTop) ||
          animation_infos_.count(AnimationPropertyType::kBottom) ||
          animation_infos_.count(AnimationPropertyType::kWidth) ||
          animation_infos_.count(AnimationPropertyType::kHeight));
}

bool TransitionManager::ContainsTransition(
    AnimationPropertyType property) const {
  return !animation_infos_.empty() &&
         animation_infos_.find(property) != animation_infos_.end();
}

void TransitionManager::StartTransitions() {
  for (auto it = running_animators_.begin(); it != running_animators_.end();) {
    if (!it->second || it->second->IsFinished()) {
      it = running_animators_.erase(it);
    } else {
      ++it;
    }
  }

  if (idle_animators_.empty()) {
    return;
  }

  auto idle_animators = std::move(idle_animators_);
  idle_animators_.clear();
  for (auto& [prop, animator] : idle_animators) {
    if (!animator) {
      continue;
    }
    auto& running = running_animators_[prop];
    if (running) {
      running->Cancel();
    }
    animator->Start();
    running = std::move(animator);
  }
}

void TransitionManager::EndTransitionAnimator(AnimationPropertyType prop) {
  if (auto idle_it = idle_animators_.find(prop);
      idle_it != idle_animators_.end()) {
    idle_animators_.erase(idle_it);
  }
  if (auto running_it = running_animators_.find(prop);
      running_it != running_animators_.end() && running_it->second) {
    running_it->second->Cancel();
    running_animators_.erase(running_it);
  }
}

void TransitionManager::RemoveDuplicateAnimation(AnimationPropertyType lhs,
                                                 AnimationPropertyType rhs) {
  if (animation_infos_.count(lhs) && animation_infos_.count(rhs)) {
    if (animation_infos_[lhs].GetOrderIndex() <
        animation_infos_[rhs].GetOrderIndex()) {
      animation_infos_.erase(lhs);
    } else {
      animation_infos_.erase(rhs);
    }
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
