// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/css_transition_manager.h"

#include <utility>

#include "core/animation/animation_trace_event_def.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {
namespace {

base::flex_optional<tasm::CSSValue> ConvertLengthToTransitionCSSValue(
    const starlight::NLength& length) {
  switch (length.GetType()) {
    case starlight::NLengthType::kNLengthAuto:
      return tasm::CSSValue(starlight::LengthValueType::kAuto);
    case starlight::NLengthType::kNLengthUnit:
      return tasm::CSSValue(length.GetRawValue(),
                            tasm::CSSValuePattern::NUMBER);
    case starlight::NLengthType::kNLengthPercentage:
      return tasm::CSSValue(length.NumericLength().GetPercentagePart(),
                            tasm::CSSValuePattern::PERCENT);
    case starlight::NLengthType::kNLengthCalc:
      // Temporary transition-only compromise: freeze pure fixed / percentage
      // calc values and defer mixed calc fidelity to a follow-up.
      if (!length.NumericLength().ContainsPercentage()) {
        return tasm::CSSValue(length.NumericLength().GetFixedPart(),
                              tasm::CSSValuePattern::NUMBER);
      }
      if (!length.NumericLength().ContainsFixedValue()) {
        return tasm::CSSValue(length.NumericLength().GetPercentagePart(),
                              tasm::CSSValuePattern::PERCENT);
      }
      return {};
    case starlight::NLengthType::kNLengthMaxContent:
    case starlight::NLengthType::kNLengthFitContent:
    case starlight::NLengthType::kNLengthFr:
      return {};
  }
  return {};
}

struct TransitionArrayLengthComponent {
  lepus::Value value;
  tasm::CSSValuePattern pattern;
};

base::flex_optional<TransitionArrayLengthComponent>
ConvertLengthToTransitionArrayComponent(const starlight::NLength& length) {
  auto css_value = ConvertLengthToTransitionCSSValue(length);
  if (!css_value) {
    return {};
  }

  switch (css_value->GetPattern()) {
    case tasm::CSSValuePattern::NUMBER:
    case tasm::CSSValuePattern::PERCENT:
      return TransitionArrayLengthComponent{css_value->GetValue(),
                                            css_value->GetPattern()};
    default:
      return {};
  }
}

base::flex_optional<TransitionArrayLengthComponent>
ConvertComputedTransformLengthToRawComponent(const starlight::NLength& length) {
  switch (length.GetType()) {
    case starlight::NLengthType::kNLengthUnit:
      return TransitionArrayLengthComponent{lepus::Value(length.GetRawValue()),
                                            tasm::CSSValuePattern::NUMBER};
    case starlight::NLengthType::kNLengthPercentage:
      return TransitionArrayLengthComponent{
          lepus::Value(length.NumericLength().GetPercentagePart()),
          tasm::CSSValuePattern::PERCENT};
    case starlight::NLengthType::kNLengthCalc:
      if (length.NumericLength().ContainsFixedValue() &&
          !length.NumericLength().ContainsPercentage()) {
        return TransitionArrayLengthComponent{
            lepus::Value(length.NumericLength().GetFixedPart()),
            tasm::CSSValuePattern::NUMBER};
      }
      if (!length.NumericLength().ContainsFixedValue() &&
          length.NumericLength().ContainsPercentage()) {
        return TransitionArrayLengthComponent{
            lepus::Value(length.NumericLength().GetPercentagePart()),
            tasm::CSSValuePattern::PERCENT};
      }
      return {};
    case starlight::NLengthType::kNLengthAuto:
    case starlight::NLengthType::kNLengthMaxContent:
    case starlight::NLengthType::kNLengthFitContent:
    case starlight::NLengthType::kNLengthFr:
      return {};
  }
  return {};
}

bool AppendComputedTransformLength(const fml::RefPtr<lepus::CArray>& item,
                                   const starlight::NLength& length) {
  auto component = ConvertComputedTransformLengthToRawComponent(length);
  if (!component) {
    return false;
  }
  item->emplace_back(std::move(component->value));
  item->emplace_back(static_cast<int>(component->pattern));
  return true;
}

base::flex_optional<tasm::CSSValue> ConvertComputedTransformForTransition(
    const starlight::CanonicalComputedValue::TransformValue& transform_value,
    const tasm::CssMeasureContext& context) {
  auto items = lepus::CArray::Create();
  items->reserve(transform_value.size());

  const auto layouts_unit_per_px = context.layouts_unit_per_px_;
  for (const auto& transform : transform_value) {
    auto item = lepus::CArray::Create();
    item->emplace_back(static_cast<int>(transform.type));
    switch (transform.type) {
      case starlight::TransformType::kTranslate:
        if (!AppendComputedTransformLength(item, transform.p0) ||
            !AppendComputedTransformLength(item, transform.p1)) {
          return {};
        }
        break;
      case starlight::TransformType::kTranslateX:
      case starlight::TransformType::kTranslateY:
      case starlight::TransformType::kTranslateZ:
        if (!AppendComputedTransformLength(item, transform.p0)) {
          return {};
        }
        break;
      case starlight::TransformType::kTranslate3d:
        if (!AppendComputedTransformLength(item, transform.p0) ||
            !AppendComputedTransformLength(item, transform.p1) ||
            !AppendComputedTransformLength(item, transform.p2)) {
          return {};
        }
        break;
      case starlight::TransformType::kRotate:
      case starlight::TransformType::kRotateX:
      case starlight::TransformType::kRotateY:
      case starlight::TransformType::kRotateZ:
        item->emplace_back(transform.p0.GetRawValue());
        break;
      case starlight::TransformType::kScale:
        item->emplace_back(transform.p0.GetRawValue());
        item->emplace_back(transform.p1.GetRawValue());
        break;
      case starlight::TransformType::kScaleX:
      case starlight::TransformType::kScaleY:
        item->emplace_back(transform.p0.GetRawValue());
        break;
      case starlight::TransformType::kSkew:
        item->emplace_back(transform.p0.GetRawValue());
        item->emplace_back(transform.p1.GetRawValue());
        break;
      case starlight::TransformType::kSkewX:
      case starlight::TransformType::kSkewY:
        item->emplace_back(transform.p0.GetRawValue());
        break;
      case starlight::TransformType::kMatrix:
        for (int i = 0; i < 6; ++i) {
          const auto value =
              transform.matrix
                  [starlight::TransformRawData::INDEX_2D_TO_3D_MATRIX_ID[i]];
          item->emplace_back((i >= 4 && layouts_unit_per_px > 0.0f)
                                 ? value / layouts_unit_per_px
                                 : value);
        }
        break;
      case starlight::TransformType::kMatrix3d:
        for (int i = 0; i < 16; ++i) {
          const auto value = transform.matrix[i];
          item->emplace_back(
              ((i == 12 || i == 13 || i == 14) && layouts_unit_per_px > 0.0f)
                  ? value / layouts_unit_per_px
                  : value);
        }
        break;
      case starlight::TransformType::kNone:
        break;
    }
    items->emplace_back(std::move(item));
  }
  return tasm::CSSValue(std::move(items));
}

tasm::CSSValue MakeTransitionBackgroundPositionDefaultValue() {
  auto inner_array = lepus::CArray::Create();
  inner_array->emplace_back(static_cast<int>(tasm::CSSValuePattern::PERCENT));
  inner_array->emplace_back(0.0);
  inner_array->emplace_back(static_cast<int>(tasm::CSSValuePattern::PERCENT));
  inner_array->emplace_back(0.0);

  auto outer_array = lepus::CArray::Create();
  outer_array->emplace_back(std::move(inner_array));
  return tasm::CSSValue(std::move(outer_array));
}

base::flex_optional<tasm::CSSValue> ConvertComputedFilterForTransition(
    const starlight::FilterData& filter) {
  switch (filter.type) {
    case starlight::FilterType::kNone:
      return tasm::CSSValue();
    case starlight::FilterType::kBlur:
    case starlight::FilterType::kGrayscale:
    case starlight::FilterType::kBrightness:
    case starlight::FilterType::kContrast:
    case starlight::FilterType::kSaturate:
      break;
    default:
      return {};
  }

  auto amount = ConvertLengthToTransitionArrayComponent(filter.amount);
  if (!amount) {
    return {};
  }

  auto array = lepus::CArray::Create();
  array->emplace_back(static_cast<int>(filter.type));
  array->emplace_back(std::move(amount->value));
  array->emplace_back(static_cast<int>(amount->pattern));
  return tasm::CSSValue(std::move(array));
}

}  // namespace

const char* ConvertAnimationPropertyTypeToString(
    lynx::starlight::AnimationPropertyType type) {
  switch (type) {
    case starlight::AnimationPropertyType::kNone:
      return "none";
    case starlight::AnimationPropertyType::kOpacity:
      return "opacity";
    case starlight::AnimationPropertyType::kScaleX:
      return "scaleX";
    case starlight::AnimationPropertyType::kScaleY:
      return "scaleY";
    case starlight::AnimationPropertyType::kScaleXY:
      return "scaleXY";
    case starlight::AnimationPropertyType::kWidth:
      return "width";
    case starlight::AnimationPropertyType::kHeight:
      return "height";
    case starlight::AnimationPropertyType::kBackgroundColor:
      return "background-color";
    case starlight::AnimationPropertyType::kColor:
      return "color";
    case starlight::AnimationPropertyType::kVisibility:
      return "visibility";
    case starlight::AnimationPropertyType::kLeft:
      return "left";
    case starlight::AnimationPropertyType::kTop:
      return "top";
    case starlight::AnimationPropertyType::kRight:
      return "right";
    case starlight::AnimationPropertyType::kBottom:
      return "bottom";
    case starlight::AnimationPropertyType::kTransform:
      return "transform";
    case starlight::AnimationPropertyType::kAll:
    case starlight::AnimationPropertyType::kLegacyAll_3:
    case starlight::AnimationPropertyType::kLegacyAll_2:
    case starlight::AnimationPropertyType::kLegacyAll_1:
      return "all";
    case starlight::AnimationPropertyType::kMaxWidth:
      return "max-width";
    case starlight::AnimationPropertyType::kMinWidth:
      return "min-width";
    case starlight::AnimationPropertyType::kMaxHeight:
      return "max-height";
    case starlight::AnimationPropertyType::kMinHeight:
      return "min-height";
    case starlight::AnimationPropertyType::kMarginLeft:
      return "margin-left";
    case starlight::AnimationPropertyType::kMarginRight:
      return "margin-right";
    case starlight::AnimationPropertyType::kMarginTop:
      return "margin-top";
    case starlight::AnimationPropertyType::kMarginBottom:
      return "margin-bottom";
    case starlight::AnimationPropertyType::kPaddingLeft:
      return "padding-left";
    case starlight::AnimationPropertyType::kPaddingRight:
      return "padding-right";
    case starlight::AnimationPropertyType::kPaddingTop:
      return "padding-top";
    case starlight::AnimationPropertyType::kPaddingBottom:
      return "padding-bottom";
    case starlight::AnimationPropertyType::kBorderLeftWidth:
      return "border-left-width";
    case starlight::AnimationPropertyType::kBorderLeftColor:
      return "border-left-color";
    case starlight::AnimationPropertyType::kBorderRightWidth:
      return "border-right-width";
    case starlight::AnimationPropertyType::kBorderRightColor:
      return "border-right-color";
    case starlight::AnimationPropertyType::kBorderTopWidth:
      return "border-top-width";
    case starlight::AnimationPropertyType::kBorderTopColor:
      return "border-top-color";
    case starlight::AnimationPropertyType::kBorderBottomWidth:
      return "border-bottom-width";
    case starlight::AnimationPropertyType::kBorderBottomColor:
      return "border-bottom-color";
    case starlight::AnimationPropertyType::kFlexBasis:
      return "flex-basis";
    case starlight::AnimationPropertyType::kFlexGrow:
      return "flex-grow";
    case starlight::AnimationPropertyType::kBorderWidth:
      return "border-width";
    case starlight::AnimationPropertyType::kBorderColor:
      return "border-color";
    case starlight::AnimationPropertyType::kMargin:
      return "margin";
    case starlight::AnimationPropertyType::kPadding:
      return "padding";
    case starlight::AnimationPropertyType::kFilter:
      return "filter";
    case starlight::AnimationPropertyType::kOffsetDistance:
      return "offset-distance";
    case starlight::AnimationPropertyType::kBoxShadow:
      return "box-shadow";
    case starlight::AnimationPropertyType::kBackgroundPosition:
      return "background-position";
    case starlight::AnimationPropertyType::kTransformOrigin:
      return "transform-origin";
    default:
      return "";
  }
}

base::flex_optional<tasm::CSSValue> ConvertCanonicalComputedValueForTransition(
    tasm::CSSPropertyID css_id, const starlight::CanonicalComputedValue& value,
    const tasm::CssMeasureContext& context) {
  using Kind = starlight::CanonicalComputedValue::Kind;
  switch (css_id) {
    case tasm::kPropertyIDLeft:
    case tasm::kPropertyIDTop:
    case tasm::kPropertyIDRight:
    case tasm::kPropertyIDBottom:
    case tasm::kPropertyIDWidth:
    case tasm::kPropertyIDHeight:
    case tasm::kPropertyIDMaxWidth:
    case tasm::kPropertyIDMinWidth:
    case tasm::kPropertyIDMaxHeight:
    case tasm::kPropertyIDMinHeight:
    case tasm::kPropertyIDMarginLeft:
    case tasm::kPropertyIDMarginRight:
    case tasm::kPropertyIDMarginTop:
    case tasm::kPropertyIDMarginBottom:
    case tasm::kPropertyIDPaddingLeft:
    case tasm::kPropertyIDPaddingRight:
    case tasm::kPropertyIDPaddingTop:
    case tasm::kPropertyIDPaddingBottom:
    case tasm::kPropertyIDFlexBasis:
      if (value.kind() != Kind::kLength) {
        return {};
      }
      if (const auto* length =
              std::get_if<starlight::CanonicalComputedValue::kLengthIndex>(
                  &value.storage())) {
        return ConvertLengthToTransitionCSSValue(*length);
      }
      return {};
    case tasm::kPropertyIDBorderLeftWidth:
    case tasm::kPropertyIDBorderRightWidth:
    case tasm::kPropertyIDBorderTopWidth:
    case tasm::kPropertyIDBorderBottomWidth:
      if (value.kind() != Kind::kResolvedLength) {
        return {};
      }
      if (const auto* resolved_length =
              std::get_if<starlight::CanonicalComputedValue::kFloatIndex>(
                  &value.storage())) {
        return tasm::CSSValue(*resolved_length, tasm::CSSValuePattern::NUMBER);
      }
      return {};
    case tasm::kPropertyIDOpacity:
    case tasm::kPropertyIDFlexGrow:
    case tasm::kPropertyIDOffsetDistance:
      if (value.kind() != Kind::kNumber) {
        return {};
      }
      if (const auto* number =
              std::get_if<starlight::CanonicalComputedValue::kFloatIndex>(
                  &value.storage())) {
        return tasm::CSSValue(*number, tasm::CSSValuePattern::NUMBER);
      }
      return {};
    case tasm::kPropertyIDBackgroundColor:
    case tasm::kPropertyIDColor:
    case tasm::kPropertyIDBorderLeftColor:
    case tasm::kPropertyIDBorderRightColor:
    case tasm::kPropertyIDBorderTopColor:
    case tasm::kPropertyIDBorderBottomColor:
      if (value.kind() != Kind::kColor) {
        return {};
      }
      if (const auto* color =
              std::get_if<starlight::CanonicalComputedValue::kColorIndex>(
                  &value.storage())) {
        return tasm::CSSValue(*color, tasm::CSSValuePattern::NUMBER);
      }
      return {};
    case tasm::kPropertyIDTransform:
      if (value.kind() != Kind::kTransform) {
        return {};
      }
      {
        const auto* transform_value =
            std::get_if<starlight::CanonicalComputedValue::kTransformIndex>(
                &value.storage());
        if (transform_value == nullptr) {
          return {};
        }
        if (transform_value->empty()) {
          return CSSKeyframeManager::GetDefaultValue(
              starlight::AnimationPropertyType::kTransform);
        }
        return ConvertComputedTransformForTransition(*transform_value, context);
      }
    case tasm::kPropertyIDFilter: {
      if (value.kind() != Kind::kFilter) {
        return {};
      }
      const auto* filter =
          std::get_if<starlight::CanonicalComputedValue::kFilterIndex>(
              &value.storage());
      if (filter == nullptr) {
        return {};
      }
      if (filter->type == starlight::FilterType::kNone) {
        return tasm::CSSValue();
      }
      return ConvertComputedFilterForTransition(*filter);
    }
    case tasm::kPropertyIDBackgroundPosition: {
      if (value.kind() != Kind::kBackgroundPosition) {
        return {};
      }
      const auto* positions = std::get_if<
          starlight::CanonicalComputedValue::kBackgroundPositionIndex>(
          &value.storage());
      if (positions == nullptr) {
        return {};
      }
      if (positions->empty()) {
        return MakeTransitionBackgroundPositionDefaultValue();
      }
      if ((positions->size() % 2) != 0) {
        return {};
      }
      auto outer_array = lepus::CArray::Create();
      for (size_t index = 0; index < positions->size(); index += 2) {
        auto x = ConvertLengthToTransitionArrayComponent((*positions)[index]);
        auto y =
            ConvertLengthToTransitionArrayComponent((*positions)[index + 1]);
        if (!x || !y) {
          return {};
        }
        auto inner_array = lepus::CArray::Create();
        inner_array->emplace_back(static_cast<int>(x->pattern));
        inner_array->emplace_back(x->value);
        inner_array->emplace_back(static_cast<int>(y->pattern));
        inner_array->emplace_back(y->value);
        outer_array->emplace_back(std::move(inner_array));
      }
      return tasm::CSSValue(std::move(outer_array));
    }
    case tasm::kPropertyIDTransformOrigin: {
      if (value.kind() != Kind::kTransformOrigin) {
        return {};
      }
      const auto* transform_origin =
          std::get_if<starlight::CanonicalComputedValue::kTransformOriginIndex>(
              &value.storage());
      if (transform_origin == nullptr) {
        return {};
      }
      auto x = ConvertLengthToTransitionArrayComponent(transform_origin->x);
      auto y = ConvertLengthToTransitionArrayComponent(transform_origin->y);
      if (!x || !y) {
        return {};
      }
      auto array = lepus::CArray::Create();
      array->emplace_back(x->value);
      array->emplace_back(static_cast<int>(x->pattern));
      array->emplace_back(y->value);
      array->emplace_back(static_cast<int>(y->pattern));
      return tasm::CSSValue(std::move(array));
    }
    case tasm::kPropertyIDVisibility: {
      if (value.kind() != Kind::kEnum) {
        return {};
      }
      const auto* enum_value =
          std::get_if<starlight::CanonicalComputedValue::kEnumIndex>(
              &value.storage());
      if (enum_value == nullptr) {
        return {};
      }
      return tasm::CSSValue(*enum_value, tasm::CSSValuePattern::ENUM);
    }
    default:
      return {};
  }
}

void CSSTransitionManager::setTransitionData(
    const starlight::TransitionData& transition_data) {
  transition_data_.clear();
  property_types_.clear();
  base::LinearFlatMap<base::String, std::shared_ptr<Animation>>
      active_animations_map;

  for (auto proxy : transition_data) {
    starlight::AnimationPropertyType property = proxy.property();
    long duration = proxy.duration();
    long delay = proxy.delay();
    starlight::TimingFunctionData tf = proxy.timing_func();

    if (property == starlight::AnimationPropertyType::kAll ||
        property == starlight::AnimationPropertyType::kLegacyAll_1 ||
        property == starlight::AnimationPropertyType::kLegacyAll_2 ||
        property == starlight::AnimationPropertyType::kLegacyAll_3) {
      const auto& transition_props_map =
          GetPropertyIDToAnimationPropertyTypeMap();
      for (const auto& iterator : transition_props_map) {
        SetTransitionDataInternal(iterator.second, duration, delay, tf,
                                  active_animations_map);
      }
    } else if (property == starlight::AnimationPropertyType::kBorderWidth ||
               property == starlight::AnimationPropertyType::kBorderColor ||
               property == starlight::AnimationPropertyType::kPadding ||
               property == starlight::AnimationPropertyType::kMargin) {
      const auto& poly_transition_props_map =
          GetPolymericPropertyIDToAnimationPropertyTypeMap(property);
      for (const auto& iterator : poly_transition_props_map) {
        SetTransitionDataInternal(iterator.second, duration, delay, tf,
                                  active_animations_map);
      }
    } else {
      SetTransitionDataInternal(property, duration, delay, tf,
                                active_animations_map);
    }
  }

  for (auto& animation_iterator : animations_map_) {
    animation_iterator.second->Destroy();
  }
  animations_map_.swap(active_animations_map);
}

void CSSTransitionManager::SetTransitionDataInternal(
    starlight::AnimationPropertyType property, long duration, long delay,
    const starlight::TimingFunctionData& timing_func,
    base::LinearFlatMap<base::String, std::shared_ptr<Animation>>&
        active_animations_map) {
  // 1. Constructor animation_data according to transition_data
  property_types_.emplace(static_cast<unsigned int>(property));
  auto& animation_data = transition_data_[static_cast<unsigned int>(property)];
  animation_data.name = ConvertAnimationPropertyTypeToString(property);
  animation_data.duration = duration;
  animation_data.delay = delay;
  animation_data.timing_func = timing_func;
  animation_data.iteration_count = 1;
  animation_data.fill_mode = starlight::AnimationFillModeType::kForwards;
  animation_data.direction = starlight::AnimationDirectionType::kNormal;
  animation_data.play_state = starlight::AnimationPlayStateType::kRunning;

  // 2. Update data to the existing animation, and temporarily save them to
  // active_animations_map.
  auto animation = animations_map_.find(animation_data.name);
  if (animation != animations_map_.end()) {
    // Add it to active_animations_map and delete it from animations_map_;
    // Unlike keyframes, transitions do not require updating the animation
    // parameter of existing animator.
    active_animations_map[animation_data.name] = animation->second;
    animations_map_.erase(animation);
  }
}

bool CSSTransitionManager::ConsumeCSSProperty(tasm::CSSPropertyID css_id,
                                              const tasm::CSSValue& end_value) {
  starlight::AnimationPropertyType property_type =
      GetAnimationPropertyType(css_id);
  if (IsShouldTransitionType(property_type)) {
    // 1. get transition start value and end value
    tasm::CSSValue start_value_internal;
    tasm::CSSValue end_value_internal;
    std::optional<tasm::CSSValue> start_value_opt =
        element()->GetElementPreviousStyle(css_id);
    if (!start_value_opt || start_value_opt->IsEmpty()) {
      // If the start value is empty, we should give it a default value rather
      // than return directly.
      start_value_internal = GetDefaultValue(property_type);
    } else {
      start_value_internal = std::move(*start_value_opt);
    }

    if (end_value.IsEmpty()) {
      // If the end value is empty, we should give it a default value rather
      // than return directly.
      end_value_internal = GetDefaultValue(property_type);
    } else {
      end_value_internal = end_value;
    }
    const auto& configs = element()->element_manager()->GetCSSParserConfigs();
    if (!IsValueValid(property_type, start_value_internal, configs) ||
        !IsValueValid(property_type, end_value_internal, configs) ||
        start_value_internal == end_value_internal ||
        (previous_end_values_.contains(css_id) &&
         end_value_internal == previous_end_values_[css_id])) {
      TryToStopTransitionAnimator(property_type);
      return false;
    }
    previous_end_values_[css_id] = end_value_internal;

    // 2. construct keyframes Map
    auto start_shared_style_map = std::make_shared<tasm::StyleMap>();
    start_shared_style_map->insert_or_assign(css_id,
                                             std::move(start_value_internal));

    auto end_shared_style_map = std::make_shared<tasm::StyleMap>();
    end_shared_style_map->insert_or_assign(css_id,
                                           std::move(end_value_internal));

    keyframe_tokens_[ConvertAnimationPropertyTypeToString(property_type)] =
        tasm::CSSKeyframesContent{{0.f, std::move(start_shared_style_map)},
                                  {1.f, std::move(end_shared_style_map)}};

    // 3. create transition animation and play
    const auto& data =
        transition_data_.find(static_cast<unsigned int>(property_type));
    if (data != transition_data_.end()) {
      if (animations_map_.count(data->second.name)) {
        // If a transition animation is replaced by another identical transition
        // animation (both animate the same properties), then this transition
        // animation does not require clearing effect and applying the end
        // effect.
        animations_map_[data->second.name]->Destroy(false);
      }
      std::shared_ptr<Animation> animation = CreateAnimation(data->second);
      animation->BindDelegate(this);
      animation->SetTransitionFlag();
      animation->Play();
      animations_map_[data->second.name] = std::move(animation);
      return true;
    }
  }
  return false;
}

void CSSTransitionManager::TickAllAnimation(fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TRANSITION_MANAGER_NEEDS_ANIMATION_RECALC);
  CSSKeyframeManager::TickAllAnimation(frame_time);
  // After traversing the set, the final_animator_maps_ is now assembled.
}

const tasm::CSSKeyframesContent& CSSTransitionManager::GetKeyframesStyleMap(
    const base::String& animation_name) {
  auto it = keyframe_tokens_.find(animation_name);
  if (it != keyframe_tokens_.end()) {
    return it->second;
  }

  return GetEmptyKeyframeMap();
}

void CSSTransitionManager::TryToStopTransitionAnimator(
    starlight::AnimationPropertyType property_type) {
  const auto& data =
      transition_data_.find(static_cast<unsigned int>(property_type));
  if (data == transition_data_.end()) {
    return;
  }
  const auto& animation_iterator = animations_map_.find(data->second.name);
  if (animation_iterator == animations_map_.end()) {
    return;
  }
  animation_iterator->second->Destroy();
  animations_map_.erase(animation_iterator);
}

bool CSSTransitionManager::IsValueValid(starlight::AnimationPropertyType type,
                                        const tasm::CSSValue& value,
                                        const tasm::CSSParserConfigs& configs) {
  switch (type) {
    case starlight::AnimationPropertyType::kWidth:
    case starlight::AnimationPropertyType::kHeight:
    case starlight::AnimationPropertyType::kLeft:
    case starlight::AnimationPropertyType::kTop:
    case starlight::AnimationPropertyType::kRight:
    case starlight::AnimationPropertyType::kBottom:
    case starlight::AnimationPropertyType::kMaxWidth:
    case starlight::AnimationPropertyType::kMinWidth:
    case starlight::AnimationPropertyType::kMaxHeight:
    case starlight::AnimationPropertyType::kMinHeight:
    case starlight::AnimationPropertyType::kPaddingLeft:
    case starlight::AnimationPropertyType::kPaddingRight:
    case starlight::AnimationPropertyType::kPaddingTop:
    case starlight::AnimationPropertyType::kPaddingBottom:
    case starlight::AnimationPropertyType::kMarginLeft:
    case starlight::AnimationPropertyType::kMarginRight:
    case starlight::AnimationPropertyType::kMarginTop:
    case starlight::AnimationPropertyType::kMarginBottom:
    case starlight::AnimationPropertyType::kBorderLeftWidth:
    case starlight::AnimationPropertyType::kBorderRightWidth:
    case starlight::AnimationPropertyType::kBorderTopWidth:
    case starlight::AnimationPropertyType::kBorderBottomWidth:
    case starlight::AnimationPropertyType::kFlexBasis: {
      auto parse_result = starlight::CSSStyleUtils::ToLength(
          value, CSSKeyframeManager::GetLengthContext(element()), configs);
      // return directly if the value of layout property is auto
      if (!parse_result.second) {
        return false;
      }
      if (!parse_result.first.IsUnit() && !parse_result.first.IsPercent() &&
          !parse_result.first.IsCalc() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kOpacity: {
      if (!value.IsNumber() && !value.IsVariable()) {
        return false;
      }
      if (value.IsNumber()) {
        auto parse_result = value.GetNumber();
        if (parse_result < 0 || parse_result > 1) {
          return false;
        }
      }
      return true;
    }
    case starlight::AnimationPropertyType::kFlexGrow:
    case starlight::AnimationPropertyType::kOffsetDistance: {
      if (!value.IsNumber() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kColor:
    case starlight::AnimationPropertyType::kBackgroundColor:
    case starlight::AnimationPropertyType::kBorderLeftColor:
    case starlight::AnimationPropertyType::kBorderRightColor:
    case starlight::AnimationPropertyType::kBorderTopColor:
    case starlight::AnimationPropertyType::kBorderBottomColor: {
      if (!value.IsNumber() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kTransform: {
      if (!value.IsArray() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kFilter: {
      if (!value.IsArray() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kBackgroundPosition: {
      if (!value.IsArray() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kTransformOrigin: {
      if (!value.IsArray() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kVisibility: {
      if (!value.IsEnum() && !value.IsVariable()) {
        return false;
      }
      return true;
    }
    default: {
      return false;
    }
  }
}

starlight::AnimationPropertyType CSSTransitionManager::GetAnimationPropertyType(
    tasm::CSSPropertyID id) {
  const auto& transition_props_map = GetPropertyIDToAnimationPropertyTypeMap();
  const auto& property_it = transition_props_map.find(id);
  if (property_it != transition_props_map.end()) {
    return property_it->second;
  }
  return starlight::AnimationPropertyType::kNone;
}

bool CSSTransitionManager::NeedsTransition(tasm::CSSPropertyID css_id) {
  starlight::AnimationPropertyType property_type =
      GetAnimationPropertyType(css_id);
  return IsShouldTransitionType(property_type);
}

bool CSSTransitionManager::IsShouldTransitionType(
    starlight::AnimationPropertyType type) {
  return property_types_.find(static_cast<unsigned int>(type)) !=
         property_types_.end();
}

void CSSTransitionManager::ClearPreviousEndValue(tasm::CSSPropertyID css_id) {
  previous_end_values_.erase(css_id);
}

}  // namespace animation
}  // namespace lynx
