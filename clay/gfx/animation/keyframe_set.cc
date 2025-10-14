// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/keyframe_set.h"

#include <limits>
#include <optional>
#include <sstream>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/animator_target.h"
#include "clay/gfx/animation/keyframe.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/type_evaluator.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

namespace {

// template function used to sort keyframes
template <typename KeyframeType>
void InsertKeyframe(std::unique_ptr<KeyframeType> keyframe,
                    std::vector<std::unique_ptr<KeyframeType>>* keyframes) {
  // Usually, the keyframes will be added in order, so this loop would be
  // unnecessary and we should skip it if possible.
  if (!keyframes->empty() &&
      keyframe->GetFraction() < keyframes->back()->GetFraction()) {
    for (size_t i = 0; i < keyframes->size(); ++i) {
      if (keyframe->GetFraction() < keyframes->at(i)->GetFraction()) {
        keyframes->insert(keyframes->begin() + i, std::move(keyframe));
        return;
      }
    }
  }

  keyframes->push_back(std::move(keyframe));
}

/**
 * Gets the animated value, given the elapsed fraction of the animation
 * (interpolated by the animation's interpolator) and the evaluator used
 * to calculate in-between values. This function maps the input fraction
 * to the appropriate keyframe interval and a fraction between them and
 * returns the interpolated value. Note that the input fraction may fall
 * outside the [0-1] bounds, if the animation's interpolator made that
 * happen (e.g., a spring interpolation that might send the fraction past
 * 1.0). We handle this situation by just using the two keyframes at the
 * appropriate end when the value is outside those bounds.
 *
 * @param fraction The elapsed fraction of the animation
 * @return The animated value.
 */
template <typename KeyframeType>
typename KeyframeType::ValueType GetValue(
    float fraction,
    const std::vector<std::unique_ptr<KeyframeType>>& keyframes) {
  FML_DCHECK(keyframes.front()->GetFraction() == 0.f);
  FML_DCHECK(keyframes.back()->GetFraction() == 1.f);
  for (size_t i = 1; i < keyframes.size(); ++i) {
    const auto& prev_keyframe = keyframes[i - 1];
    float prev_fraction = prev_keyframe->GetFraction();
    const auto& next_keyframe = keyframes[i];
    if (fraction <= next_keyframe->GetFraction() || i == keyframes.size() - 1) {
      Interpolator* interpolator = prev_keyframe->GetInterpolator();
      float intervalFraction = (fraction - prev_fraction) /
                               (next_keyframe->GetFraction() - prev_fraction);

      // Apply interpolator on the proportional duration.
      if (interpolator != nullptr) {
        intervalFraction = interpolator->Interpolate(intervalFraction);
      }
      return TypeEvaluator<typename KeyframeType::ValueType>::Evaluate(
          intervalFraction, prev_keyframe->Value(), next_keyframe->Value());
    }
  }
  FML_UNREACHABLE();
  return keyframes.back()->Value();
}

}  // namespace

KeyframeSet::~KeyframeSet() = default;

KeyframeSet::KeyframeSet(ClayAnimationPropertyType property_type)
    : property_type_(property_type) {}

void KeyframeSet::SetKeyframesManager(KeyframesManager* keyframes_manager) {
  keyframes_manager_ = keyframes_manager;
}

std::unique_ptr<FloatKeyframeSet> FloatKeyframeSet::Create(
    ClayAnimationPropertyType property_type) {
  return std::unique_ptr<FloatKeyframeSet>(new FloatKeyframeSet(property_type));
}

FloatKeyframeSet::FloatKeyframeSet(ClayAnimationPropertyType property_type)
    : KeyframeSet(property_type) {}

FloatKeyframeSet::~FloatKeyframeSet() = default;

void FloatKeyframeSet::AddKeyframe(std::unique_ptr<FloatKeyframe> keyframe) {
  InsertKeyframe(std::move(keyframe), &keyframes_);
}

std::unique_ptr<KeyframeSet> FloatKeyframeSet::Clone(
    KeyframesManager* manager) const {
  std::unique_ptr<FloatKeyframeSet> to_return =
      FloatKeyframeSet::Create(Type());
  to_return->SetKeyframesManager(manager);
  for (const auto& keyframe : keyframes_) {
    to_return->AddKeyframe(keyframe->Clone());
  }

  return to_return;
}

void FloatKeyframeSet::OnAnimationStart(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->GetProperty(Type(), original_value_);
  }
}

void FloatKeyframeSet::OnAnimationRemove(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->SetProperty(Type(), original_value_, false);
  }
}

void FloatKeyframeSet::OnAnimationUpdate(ValueAnimator& animation) {
  if (keyframes_.front()->GetFraction() > 0) {
    auto* interpolator = keyframes_.front()->GetInterpolator();
    AddKeyframe(
        FloatKeyframe::Create(0.f, original_value_, interpolator->Clone()));
  }
  if (keyframes_.back()->GetFraction() < 1) {
    auto* interpolator = keyframes_.back()->GetInterpolator();
    AddKeyframe(
        FloatKeyframe::Create(1.f, original_value_, interpolator->Clone()));
  }
  if (auto manager = GetKeyframesManager()) {
    float current_fraction = animation.GetAnimatedFraction();
    manager->GetTarget()->SetProperty(Type(), GetValue(current_fraction), true);
  }
}

float FloatKeyframeSet::GetValue(float fraction) const {
  return clay::GetValue(fraction, keyframes_);
}

#ifndef NDEBUG
std::string FloatKeyframeSet::ToString() const {
  std::ostringstream os;
  os << "FloatKeyframeSet: size=" << keyframes_.size()
     << " type=" << static_cast<int>(Type()) << std::endl;
  for (const auto& keyframe : keyframes_) {
    os << "\t" << keyframe->ToString() << std::endl;
  }
  return os.str();
}
#endif

std::unique_ptr<ColorKeyframeSet> ColorKeyframeSet::Create(
    ClayAnimationPropertyType property_type) {
  return std::unique_ptr<ColorKeyframeSet>(new ColorKeyframeSet(property_type));
}

ColorKeyframeSet::ColorKeyframeSet(ClayAnimationPropertyType property_type)
    : KeyframeSet(property_type) {}

ColorKeyframeSet::~ColorKeyframeSet() = default;

void ColorKeyframeSet::AddKeyframe(std::unique_ptr<ColorKeyframe> keyframe) {
  InsertKeyframe(std::move(keyframe), &keyframes_);
}

std::unique_ptr<KeyframeSet> ColorKeyframeSet::Clone(
    KeyframesManager* manager) const {
  std::unique_ptr<ColorKeyframeSet> to_return =
      ColorKeyframeSet::Create(Type());
  to_return->SetKeyframesManager(manager);
  for (const auto& keyframe : keyframes_) {
    to_return->AddKeyframe(keyframe->Clone());
  }

  return to_return;
}

void ColorKeyframeSet::OnAnimationStart(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->GetProperty(Type(), original_value_);
  }
}

void ColorKeyframeSet::OnAnimationRemove(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->SetProperty(Type(), original_value_, false);
  }
}

void ColorKeyframeSet::OnAnimationUpdate(ValueAnimator& animation) {
  if (keyframes_.front()->GetFraction() > 0) {
    auto* interpolator = keyframes_.front()->GetInterpolator();
    AddKeyframe(
        ColorKeyframe::Create(0.f, original_value_, interpolator->Clone()));
  }
  if (keyframes_.back()->GetFraction() < 1) {
    auto* interpolator = keyframes_.back()->GetInterpolator();
    AddKeyframe(
        ColorKeyframe::Create(1.f, original_value_, interpolator->Clone()));
  }
  if (auto manager = GetKeyframesManager()) {
    float current_fraction = animation.GetAnimatedFraction();
    manager->GetTarget()->SetProperty(Type(), GetValue(current_fraction), true);
  }
}

Color ColorKeyframeSet::GetValue(float fraction) const {
  return clay::GetValue(fraction, keyframes_);
}

#ifndef NDEBUG
std::string ColorKeyframeSet::ToString() const {
  std::ostringstream os;
  os << "ColorKeyframeSet: size=" << keyframes_.size()
     << " type=" << static_cast<int>(Type()) << std::endl;
  for (const auto& keyframe : keyframes_) {
    os << "\t" << keyframe->ToString() << std::endl;
  }
  return os.str();
}
#endif

std::unique_ptr<RawTransformKeyframeSet> RawTransformKeyframeSet::Create() {
  return std::unique_ptr<RawTransformKeyframeSet>(
      new RawTransformKeyframeSet());
}

RawTransformKeyframeSet::RawTransformKeyframeSet()
    : KeyframeSet(ClayAnimationPropertyType::kTransform) {}

void RawTransformKeyframeSet::AddKeyframe(
    std::unique_ptr<RawTransformKeyframe> keyframe) {
  for (const auto& op : keyframe->Operations()) {
    switch (op.type) {
      case ClayTransformType::kTranslateX:
        has_percentage_values_ |=
            op.unit[0] == ClayPlatformLengthUnit::kPercentage;
        break;
      case ClayTransformType::kTranslateY:
        has_percentage_values_ |=
            op.unit[1] == ClayPlatformLengthUnit::kPercentage;
        break;
      case ClayTransformType::kTranslate:
      case ClayTransformType::kTranslate3d:
        has_percentage_values_ |=
            op.unit[0] == ClayPlatformLengthUnit::kPercentage;
        has_percentage_values_ |=
            op.unit[1] == ClayPlatformLengthUnit::kPercentage;
        break;
      default:
        FML_LOG(ERROR) << "Unsupported transform type "
                       << static_cast<int>(op.type);
        break;
    }
  }

  InsertKeyframe(std::move(keyframe), &keyframes_);
}

std::unique_ptr<KeyframeSet> RawTransformKeyframeSet::Clone(
    KeyframesManager* manager) const {
  std::unique_ptr<TransformKeyframeSet> to_return =
      TransformKeyframeSet::Create(Type());
  to_return->SetKeyframesManager(manager);
  for (const auto& keyframe : keyframes_) {
    ClayTransform transform;
    transform.op = const_cast<ClayTransformOP*>(keyframe->Operations().data());
    transform.size = keyframe->Operations().size();
    to_return->AddKeyframe(TransformKeyframe::Create(
        keyframe->GetFraction(),
        TransformOperations(transform,
                            manager->GetTarget()->PercentageResolutionSize()),
        keyframe->GetInterpolator()->Clone()));
  }

  return to_return;
}

#ifndef NDEBUG
std::string RawTransformKeyframeSet::ToString() const {
  std::ostringstream os;
  os << "RawTransformKeyframeSet: size=" << keyframes_.size()
     << " type=" << static_cast<int>(Type()) << std::endl;
  for (const auto& keyframe : keyframes_) {
    os << "\t" << keyframe->ToString() << std::endl;
  }
  return os.str();
}
#endif

std::unique_ptr<TransformKeyframeSet> TransformKeyframeSet::Create(
    ClayAnimationPropertyType property_type) {
  return std::unique_ptr<TransformKeyframeSet>(
      new TransformKeyframeSet(property_type));
}

TransformKeyframeSet::TransformKeyframeSet(
    ClayAnimationPropertyType property_type)
    : KeyframeSet(property_type) {}

TransformKeyframeSet::~TransformKeyframeSet() = default;

void TransformKeyframeSet::AddKeyframe(
    std::unique_ptr<TransformKeyframe> keyframe) {
  InsertKeyframe(std::move(keyframe), &keyframes_);
}

std::unique_ptr<KeyframeSet> TransformKeyframeSet::Clone(
    KeyframesManager* manager) const {
  std::unique_ptr<TransformKeyframeSet> to_return =
      TransformKeyframeSet::Create(Type());
  to_return->SetKeyframesManager(manager);
  for (const auto& keyframe : keyframes_) {
    to_return->AddKeyframe(keyframe->Clone());
  }

  return to_return;
}

void TransformKeyframeSet::OnAnimationStart(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->GetProperty(Type(), original_value_);
  }
}

void TransformKeyframeSet::OnAnimationRemove(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->SetProperty(Type(), original_value_, false);
  }
}

void TransformKeyframeSet::OnAnimationUpdate(ValueAnimator& animation) {
  if (keyframes_.front()->GetFraction() > 0) {
    auto* interpolator = keyframes_.front()->GetInterpolator();
    AddKeyframe(
        TransformKeyframe::Create(0.f, original_value_, interpolator->Clone()));
  }
  if (keyframes_.back()->GetFraction() < 1) {
    auto* interpolator = keyframes_.back()->GetInterpolator();
    AddKeyframe(
        TransformKeyframe::Create(1.f, original_value_, interpolator->Clone()));
  }
  if (auto manager = GetKeyframesManager()) {
    float current_fraction = animation.GetAnimatedFraction();
    manager->GetTarget()->SetProperty(Type(), GetValue(current_fraction), true);
  }
}

TransformOperations TransformKeyframeSet::GetValue(float fraction) const {
  return clay::GetValue(fraction, keyframes_);
}

#ifndef NDEBUG
std::string TransformKeyframeSet::ToString() const {
  std::ostringstream os;
  os << "TransformKeyframeSet: size=" << keyframes_.size()
     << " type=" << static_cast<int>(Type()) << std::endl;
  for (const auto& keyframe : keyframes_) {
    os << "\t" << keyframe->ToString() << std::endl;
  }
  return os.str();
}
#endif

std::unique_ptr<KeyframeSet> FilterKeyframeSet::Clone(
    KeyframesManager* manager) const {
  std::unique_ptr<FilterKeyframeSet> to_return = FilterKeyframeSet::Create();
  to_return->SetKeyframesManager(manager);
  for (const auto& keyframe : keyframes_) {
    to_return->AddKeyframe(
        FilterKeyframe::Create(keyframe->GetFraction(), keyframe->Value(),
                               keyframe->GetInterpolator()->Clone()));
  }
  return to_return;
}

std::unique_ptr<FilterKeyframeSet> FilterKeyframeSet::Create() {
  return std::unique_ptr<FilterKeyframeSet>(new FilterKeyframeSet());
}

FilterKeyframeSet::FilterKeyframeSet()
    : KeyframeSet(ClayAnimationPropertyType::kFilter) {}

FilterKeyframeSet::~FilterKeyframeSet() = default;

#ifndef NDEBUG
std::string FilterKeyframeSet::ToString() const { return "FilterKeyframeSet"; }
#endif

// AnimatorListenerAdapter overrides
void FilterKeyframeSet::OnAnimationStart(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->GetProperty(Type(), original_value_);
  }
}
void FilterKeyframeSet::OnAnimationUpdate(ValueAnimator& animation) {
  if (keyframes_.front()->GetFraction() > 0) {
    auto* interpolator = keyframes_.front()->GetInterpolator();
    AddKeyframe(
        FilterKeyframe::Create(0.f, original_value_, interpolator->Clone()));
  }
  if (keyframes_.back()->GetFraction() < 1) {
    auto* interpolator = keyframes_.back()->GetInterpolator();
    AddKeyframe(
        FilterKeyframe::Create(1.f, original_value_, interpolator->Clone()));
  }
  if (auto manager = GetKeyframesManager()) {
    float current_fraction = animation.GetAnimatedFraction();
    manager->GetTarget()->SetProperty(Type(), GetValue(current_fraction));
  }
}
void FilterKeyframeSet::OnAnimationRemove(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->SetProperty(Type(), original_value_);
  }
}

FilterOperations FilterKeyframeSet::GetValue(float fraction) const {
  return clay::GetValue(fraction, keyframes_);
}

void FilterKeyframeSet::AddKeyframe(std::unique_ptr<FilterKeyframe> keyframe) {
  InsertKeyframe(std::move(keyframe), &keyframes_);
}

std::unique_ptr<BoxShadowKeyframeSet> BoxShadowKeyframeSet::Create() {
  return std::unique_ptr<BoxShadowKeyframeSet>(new BoxShadowKeyframeSet());
}

BoxShadowKeyframeSet::BoxShadowKeyframeSet()
    : KeyframeSet(ClayAnimationPropertyType::kBoxShadow) {}

BoxShadowKeyframeSet::~BoxShadowKeyframeSet() = default;

std::unique_ptr<KeyframeSet> BoxShadowKeyframeSet::Clone(
    KeyframesManager* manager) const {
  std::unique_ptr<BoxShadowKeyframeSet> to_return =
      BoxShadowKeyframeSet::Create();
  to_return->SetKeyframesManager(manager);
  for (const auto& keyframe : keyframes_) {
    to_return->AddKeyframe(
        BoxShadowKeyframe::Create(keyframe->GetFraction(), keyframe->Value(),
                                  keyframe->GetInterpolator()->Clone()));
  }
  return to_return;
}

#ifndef NDEBUG
std::string BoxShadowKeyframeSet::ToString() const {
  return "BoxShadowKeyframeSet";
}
#endif

void BoxShadowKeyframeSet::OnAnimationStart(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->GetProperty(Type(), original_value_);
  }
}
void BoxShadowKeyframeSet::OnAnimationUpdate(ValueAnimator& animation) {
  if (keyframes_.front()->GetFraction() > 0) {
    auto* interpolator = keyframes_.front()->GetInterpolator();
    AddKeyframe(
        BoxShadowKeyframe::Create(0.f, original_value_, interpolator->Clone()));
  }
  if (keyframes_.back()->GetFraction() < 1) {
    auto* interpolator = keyframes_.back()->GetInterpolator();
    AddKeyframe(
        BoxShadowKeyframe::Create(1.f, original_value_, interpolator->Clone()));
  }
  if (auto manager = GetKeyframesManager()) {
    float current_fraction = animation.GetAnimatedFraction();
    manager->GetTarget()->SetProperty(Type(), GetValue(current_fraction));
  }
}
void BoxShadowKeyframeSet::OnAnimationRemove(Animator& animation) {
  if (auto manager = GetKeyframesManager()) {
    manager->GetTarget()->SetProperty(Type(), original_value_);
  }
}

BoxShadowOperations BoxShadowKeyframeSet::GetValue(float fraction) const {
  return clay::GetValue(fraction, keyframes_);
}

void BoxShadowKeyframeSet::AddKeyframe(
    std::unique_ptr<BoxShadowKeyframe> keyframe) {
  InsertKeyframe(std::move(keyframe), &keyframes_);
}

}  // namespace clay
