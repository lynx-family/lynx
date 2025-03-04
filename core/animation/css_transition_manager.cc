// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/css_transition_manager.h"

#include <utility>

#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {

std::string ConvertAnimationPropertyTypeToString(
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
    case starlight::AnimationPropertyType::kBoxShadow:
      return "box-shadow";
  }
}

void CSSTransitionManager::setTransitionData(
    std::vector<starlight::TransitionData>& transition_data) {
  transition_data_.clear();
  property_types_.clear();
  std::unordered_map<base::String, std::shared_ptr<Animation>>
      active_animations_map;
  for (const auto& data : transition_data) {
    if (data.property == starlight::AnimationPropertyType::kAll ||
        data.property == starlight::AnimationPropertyType::kLegacyAll_1 ||
        data.property == starlight::AnimationPropertyType::kLegacyAll_2 ||
        data.property == starlight::AnimationPropertyType::kLegacyAll_3) {
      starlight::TransitionData temp_data(data);
      const auto& transition_props_map =
          GetPropertyIDToAnimationPropertyTypeMap();
      for (const auto& iterator : transition_props_map) {
        temp_data.property = iterator.second;
        SetTransitionDataInternal(temp_data, active_animations_map);
      }
    } else if (data.property ==
                   starlight::AnimationPropertyType::kBorderWidth ||
               data.property ==
                   starlight::AnimationPropertyType::kBorderColor ||
               data.property == starlight::AnimationPropertyType::kPadding ||
               data.property == starlight::AnimationPropertyType::kMargin) {
      starlight::TransitionData temp_data(data);
      const auto& poly_transition_props_map =
          GetPolymericPropertyIDToAnimationPropertyTypeMap(data.property);
      for (const auto& iterator : poly_transition_props_map) {
        temp_data.property = iterator.second;
        SetTransitionDataInternal(temp_data, active_animations_map);
      }
    } else {
      SetTransitionDataInternal(data, active_animations_map);
    }
  }

  // 3. All animations remaining in animations_map_ need to be destroyed.
  for (auto& animation_iterator : animations_map_) {
    animation_iterator.second->Destroy();
  }
  // 4. Swap active animations to animations_map_.
  animations_map_.swap(active_animations_map);
}

void CSSTransitionManager::SetTransitionDataInternal(
    const starlight::TransitionData& data,
    std::unordered_map<base::String, std::shared_ptr<Animation>>&
        active_animations_map) {
  // 1. Constructor animation_data according to transition_data
  property_types_.emplace(static_cast<unsigned int>(data.property));
  starlight::AnimationData animation_data;
  animation_data.name = ConvertAnimationPropertyTypeToString(data.property);
  animation_data.duration = data.duration;
  animation_data.delay = data.delay;
  animation_data.timing_func = data.timing_func;
  animation_data.iteration_count = 1;
  animation_data.fill_mode = starlight::AnimationFillModeType::kForwards;
  animation_data.direction = starlight::AnimationDirectionType::kNormal;
  animation_data.play_state = starlight::AnimationPlayStateType::kRunning;

  transition_data_[static_cast<unsigned int>(data.property)] = animation_data;

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
    tasm::CSSKeyframesMap map;
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
        start_value_internal == end_value_internal) {
      TryToStopTransitionAnimator(property_type);
      return false;
    }

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSTransitionManager::TickAllAnimation");
  CSSKeyframeManager::TickAllAnimation(frame_time);
  // After traversing the set, the final_animator_maps_ is now assembled.
}

tasm::CSSKeyframesContent& CSSTransitionManager::GetKeyframesStyleMap(
    const std::string& animation_name) {
  auto it = keyframe_tokens_.find(animation_name);
  if (it != keyframe_tokens_.end()) {
    return it->second;
  }

  return empty_keyframe_map();
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
          !parse_result.first.IsCalc()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kOpacity: {
      if (!value.IsNumber()) {
        return false;
      }
      auto parse_result = value.GetValue().Number();
      if (parse_result < 0 || parse_result > 1) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kFlexGrow: {
      if (!value.IsNumber()) {
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
      if (!value.IsNumber()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kTransform: {
      if (!value.IsArray()) {
        return false;
      }
      return true;
    }
    case starlight::AnimationPropertyType::kFilter: {
      if (!value.IsArray()) {
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

}  // namespace animation
}  // namespace lynx
