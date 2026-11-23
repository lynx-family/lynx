// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/css_transition_manager.h"

#include <optional>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/value/table.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/transforms/transform_operations_helper.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace animation {
namespace {

constexpr bool kHasPlatformAnimationBackend =
    LYNX_HAS_PLATFORM_ANIMATION_BACKEND;

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

bool HasPlatformTransformContext(const starlight::ComputedCSSStyle& style) {
  const auto& values = style.GetResolvedValues();
  return style.GetOffsetPath() ||
         values.contains(tasm::kPropertyIDPerspective) ||
         values.contains(tasm::kPropertyIDTransformOrigin);
}

bool ShouldUseLayoutOnlyTransitionSource(tasm::Element* element,
                                         tasm::CSSPropertyID id) {
  return element != nullptr && !element->EnableLayoutInElementMode() &&
         tasm::LayoutProperty::IsLayoutOnly(id) &&
         starlight::ComputedCSSStyle::SupportsCanonicalComputedValue(id);
}

tasm::CSSValue GetStyleValueOrDefaultForTransition(
    const tasm::StyleMap& style_map, tasm::CSSPropertyID css_id,
    starlight::AnimationPropertyType property_type) {
  auto it = style_map.find(css_id);
  if (it != style_map.end()) {
    return it->second;
  }
  return CSSKeyframeManager::GetDefaultValue(property_type);
}

tasm::CSSValue GetResolvedLayoutOnlyStyleValueOrDefaultForTransition(
    const starlight::ComputedCSSStyle& style, tasm::CSSPropertyID css_id,
    starlight::AnimationPropertyType property_type) {
  if (tasm::LayoutProperty::IsLayoutOnly(css_id)) {
    const auto& resolved_values = style.GetResolvedValues();
    auto it = resolved_values.find(css_id);
    if (it != resolved_values.end()) {
      return it->second;
    }
  }
  return CSSKeyframeManager::GetDefaultValue(property_type);
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

base::flex_optional<tasm::CSSValue> ConvertCanonicalComputedValueForAnimation(
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
    case tasm::kPropertyIDBoxShadow: {
      if (value.kind() != Kind::kBoxShadow) {
        return {};
      }
      const auto* box_shadow =
          std::get_if<starlight::CanonicalComputedValue::kBoxShadowIndex>(
              &value.storage());
      if (box_shadow == nullptr) {
        return {};
      }
      return BuildBoxShadowCSSValue(*box_shadow);
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

base::flex_optional<tasm::CSSValue> ConvertCanonicalComputedValueForTransition(
    tasm::CSSPropertyID css_id, const starlight::CanonicalComputedValue& value,
    const tasm::CssMeasureContext& context) {
  return ConvertCanonicalComputedValueForAnimation(css_id, value, context);
}

CSSTransitionManager::CSSTransitionManager(tasm::Element* element)
    : CSSKeyframeManager(element) {}

CSSTransitionManager::~CSSTransitionManager() = default;

void CSSTransitionManager::setTransitionData(
    const starlight::TransitionData& transition_data) {
  SyncTransitionData(transition_data, true);
}

void CSSTransitionManager::SyncTransitionData(
    const starlight::TransitionData& transition_data,
    bool use_legacy_destroy_path) {
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
    if (use_legacy_destroy_path) {
      animation_iterator.second->Destroy();
    } else if (animation_iterator.second != nullptr) {
      PrepareTransitionRemovalCleanup(animation_iterator.second);
    }
  }
  animations_map_.swap(active_animations_map);
  if constexpr (kHasPlatformAnimationBackend) {
    for (auto& transition : platform_transitions_) {
      if (transition.state && !NeedsTransition(transition.property)) {
        RemovePlatformTransition(transition, true);
      }
    }
  }
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
  if (!IsShouldTransitionType(property_type)) {
    return false;
  }
  auto previous = element()->GetElementPreviousStyle(css_id);
  auto start = previous && !previous->IsEmpty()
                   ? std::move(*previous)
                   : GetDefaultValue(property_type);
  auto end = end_value.IsEmpty() ? GetDefaultValue(property_type) : end_value;
  return UpdateTransitionAnimator(css_id, property_type, std::move(start),
                                  std::move(end), true);
}

void CSSTransitionManager::UpdateTransitionsForNewPipeline(
    const starlight::ComputedCSSStyle& previous_base_style,
    const starlight::ComputedCSSStyle& previous_final_style,
    const starlight::ComputedCSSStyle& new_base_style,
    const tasm::StyleMap& previous_underlying_layout_only_styles,
    const tasm::StyleMap& new_underlying_layout_only_styles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TRANSITION_MANAGER_UPDATE_TRANSITIONS_FOR_NEW_PIPELINE);
  if (new_base_style.HasTransition()) {
    SyncTransitionData(new_base_style.transition_data(), false);
  } else {
    starlight::TransitionData empty_transition_data;
    SyncTransitionData(empty_transition_data, false);
  }

  const auto& transition_props_map = GetPropertyIDToAnimationPropertyTypeMap();
  for (const auto& [css_id, property_type] : transition_props_map) {
    if (!IsShouldTransitionType(property_type)) {
      continue;
    }

    if (ShouldUseLayoutOnlyTransitionSource(element(), css_id)) {
      const auto previous_underlying = GetStyleValueOrDefaultForTransition(
          previous_underlying_layout_only_styles, css_id, property_type);
      auto displayed_start =
          GetResolvedLayoutOnlyStyleValueOrDefaultForTransition(
              previous_final_style, css_id, property_type);
      auto new_underlying = GetStyleValueOrDefaultForTransition(
          new_underlying_layout_only_styles, css_id, property_type);

      if (previous_underlying == new_underlying) {
        continue;
      }
      if (displayed_start == new_underlying) {
        StopTransitionAnimator(css_id, property_type, false);
        continue;
      }

      UpdateTransitionAnimator(css_id, property_type,
                               std::move(displayed_start),
                               std::move(new_underlying), false);
      continue;
    }

    if (!starlight::ComputedCSSStyle::SupportsCanonicalComputedValue(css_id)) {
      continue;
    }

    // The iOS presentation matrix can include these extra transforms.
    const bool has_transform_context =
        kHasPlatformAnimationBackend && css_id == tasm::kPropertyIDTransform &&
        element()->supports_platform_animation_routing() &&
        (HasPlatformTransformContext(previous_final_style) ||
         HasPlatformTransformContext(new_base_style));
    if (has_transform_context) {
      CancelPlatformTransition(css_id);
    }

    const auto previous_underlying =
        previous_base_style.ExtractCanonicalComputedValue(css_id);
    const auto displayed_start =
        previous_final_style.ExtractCanonicalComputedValue(css_id);
    const auto new_underlying =
        new_base_style.ExtractCanonicalComputedValue(css_id);
    if (!previous_underlying || !displayed_start || !new_underlying) {
      continue;
    }

    if (*previous_underlying == *new_underlying) {
      continue;
    }
    if (*displayed_start == *new_underlying) {
      StopTransitionAnimator(css_id, property_type, false);
      continue;
    }

    auto start_value = ConvertCanonicalComputedValueForTransition(
        css_id, *displayed_start, previous_final_style.GetMeasureContext());
    auto end_value = ConvertCanonicalComputedValueForTransition(
        css_id, *new_underlying, new_base_style.GetMeasureContext());
    if (!start_value || !end_value) {
      StopTransitionAnimator(css_id, property_type, false);
      continue;
    }

    UpdateTransitionAnimator(css_id, property_type, std::move(*start_value),
                             std::move(*end_value), false,
                             !has_transform_context);
  }
}

AnimationSampleForNewPipeline
CSSTransitionManager::CollectTransitionUpdatesForNewPipeline(
    fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TRANSITION_MANAGER_COLLECT_TRANSITION_UPDATES_FOR_NEW_PIPELINE);
  if constexpr (kHasPlatformAnimationBackend) {
    ApplyReadyPlatformFallbacks();
  }
  auto sample =
      CSSKeyframeManager::CollectAnimationUpdatesForNewPipeline(frame_time);
  if constexpr (kHasPlatformAnimationBackend) {
    // An override means Core has sampled past the delay for this property.
    for (auto& transition : platform_transitions_) {
      if (transition.state &&
          sample.property_overrides.contains(transition.property)) {
        RemovePlatformTransition(transition, false);
      }
    }
  }
  return sample;
}

bool CSSTransitionManager::NeedsFutureTickForNewPipeline() const {
  return (kHasPlatformAnimationBackend &&
          (platform_transitions_[0].pending_snapshot ||
           platform_transitions_[1].pending_snapshot)) ||
         CSSKeyframeManager::NeedsFutureTickForNewPipeline();
}

void CSSTransitionManager::ApplyReadyPlatformFallbacks() {
  const auto& opacity = platform_transitions_[0];
  const auto& transform = platform_transitions_[1];
  // Preserve start-event order when both transitions fall back in one frame.
  const size_t first_slot =
      opacity.pending_snapshot && transform.pending_snapshot &&
      transform.state->animation_id < opacity.state->animation_id;
  for (size_t slot : {first_slot, 1 - first_slot}) {
    auto& transition = platform_transitions_[slot];
    auto& [css_id, state, pending_snapshot] = transition;
    const gfx::PlatformTransitionSnapshot::Value* captured = nullptr;
    if (!pending_snapshot || !pending_snapshot->Read(captured)) {
      continue;
    }

    // Transition keyframe maps keep the two endpoints in offset order.
    auto frames =
        keyframe_tokens_.find(state->animation->name())->second.begin();
    auto& start = frames[0].second->begin()->second;
    if (captured) {
      if (css_id == tasm::kPropertyIDOpacity) {
        start = tasm::CSSValue((*captured)[0], tasm::CSSValuePattern::NUMBER);
      } else {
        gfx::TransformOperations operations;
        operations.AppendMatrix(gfx::TransformOperation::kMatrix3d, *captured);
        start = transforms::ConvertToCSSValue(
            operations, GetLengthContext(element()).layouts_unit_per_px_);
      }
    }
    auto core = start == frames[1].second->begin()->second
                    ? nullptr
                    : BuildTransitionAnimation(
                          state->animation->get_animation_data(), false, false)
                          .new_animator_animation;
    if (core) {
      StartCoreTransition(std::move(core), false);
      if (captured) {
        pending_snapshot = nullptr;
        continue;
      }
    } else {
      ClearPreviousEndValue(css_id);
    }
    RemovePlatformTransition(transition, false);
  }
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

void CSSTransitionManager::StopTransitionAnimator(
    tasm::CSSPropertyID css_id, starlight::AnimationPropertyType property_type,
    bool play_handles_initial_frame) {
  const auto data =
      transition_data_.find(static_cast<unsigned int>(property_type));
  if (data == transition_data_.end()) {
    return;
  }
  const auto animation = animations_map_.find(data->second.name);
  if (animation != animations_map_.end()) {
    if (play_handles_initial_frame) {
      animation->second->Destroy();
    } else {
      PrepareTransitionRemovalCleanup(animation->second);
    }
    animations_map_.erase(animation);
  }
  if (kHasPlatformAnimationBackend && !play_handles_initial_frame) {
    CancelPlatformTransition(css_id);
  }
}

CSSTransitionManager::PlatformTransition*
CSSTransitionManager::FindPlatformTransition(tasm::CSSPropertyID css_id) {
  if (css_id == tasm::kPropertyIDOpacity) {
    return &platform_transitions_[0];
  }
  return css_id == tasm::kPropertyIDTransform ? &platform_transitions_[1]
                                              : nullptr;
}

void CSSTransitionManager::CancelPlatformTransition(
    tasm::CSSPropertyID css_id) {
  if (auto* transition = FindPlatformTransition(css_id);
      transition && transition->state) {
    RemovePlatformTransition(*transition, true);
  }
}

void CSSTransitionManager::RemovePlatformTransition(
    PlatformTransition& transition, bool clear_previous_end_value) {
  QueuePlatformAnimationCommands(*transition.state,
                                 gfx::PlatformAnimationCommandType::kCancel);
  if (clear_previous_end_value) {
    ClearPreviousEndValue(transition.property);
  }
  transition.state.reset();
  transition.pending_snapshot = nullptr;
}

void CSSTransitionManager::PrepareTransitionRemovalCleanup(
    const std::shared_ptr<Animation>& animation) {
  if (animation == nullptr) {
    return;
  }
  // New-pipeline removal bypasses Destroy(), including when replacing a target.
  animation->NotifyInspectorCanceled();
  animation->ClearTransitionPreviousEndValue();
  QueueCancelEvent(animation);
}

bool CSSTransitionManager::UpdateTransitionAnimator(
    tasm::CSSPropertyID css_id, starlight::AnimationPropertyType property_type,
    tasm::CSSValue&& start_value, tasm::CSSValue&& end_value,
    bool play_handles_initial_frame, bool allow_platform) {
  const auto& configs = element()->element_manager()->GetCSSParserConfigs();
  const auto previous_end = previous_end_values_.find(css_id);
  if (!IsValueValid(property_type, start_value, configs) ||
      !IsValueValid(property_type, end_value, configs) ||
      start_value == end_value ||
      (previous_end != previous_end_values_.end() &&
       end_value == previous_end->second)) {
    StopTransitionAnimator(css_id, property_type, play_handles_initial_frame);
    return false;
  }

  const auto& data =
      transition_data_.find(static_cast<unsigned int>(property_type));
  if (data == transition_data_.end()) {
    return false;
  }

  auto& keyframes = keyframe_tokens_[data->second.name];
  auto keyframe_value = [&](float offset) -> tasm::CSSValue& {
    auto& styles = keyframes[offset];
    if (!styles) {
      styles = std::make_shared<tasm::StyleMap>();
    }
    return (*styles)[css_id];
  };
  keyframe_value(0.f) = std::move(start_value);
  keyframe_value(1.f) = end_value;

  auto existing_new_animator_animation =
      animations_map_.find(data->second.name);
  if (existing_new_animator_animation != animations_map_.end()) {
    // If a transition animation is replaced by another identical transition
    // animation (both animate the same properties), then this transition
    // animation does not require clearing effect and applying the end effect.
    if (play_handles_initial_frame) {
      existing_new_animator_animation->second->Destroy(false);
    } else {
      PrepareTransitionRemovalCleanup(existing_new_animator_animation->second);
    }
    animations_map_.erase(existing_new_animator_animation);
  }
  // New styling commits the target independently of transition sampling.
  auto* platform =
      kHasPlatformAnimationBackend ? FindPlatformTransition(css_id) : nullptr;
  const bool has_platform = platform && platform->state;
  const bool can_route =
      platform && allow_platform &&
      !element()->element_manager()->IsFragmentLayerRenderModeOn() &&
      data->second.duration > 0 && data->second.delay >= 0;
  auto build_result =
      has_platform && !can_route
          ? AnimationBuildResult{}
          : BuildTransitionAnimation(data->second, can_route, has_platform);
  if (!build_result.new_animator_animation &&
      !build_result.platform_animation && !has_platform) {
    return false;
  }
  previous_end_values_[css_id] = std::move(end_value);
  if constexpr (kHasPlatformAnimationBackend) {
    if (build_result.platform_animation) {
      auto& next = *build_result.platform_animation;
      auto command_type = gfx::PlatformAnimationCommandType::kHandoff;
      if (has_platform) {
        next.RetainIdentityFrom(*platform->state);
        command_type = gfx::PlatformAnimationCommandType::kUpdate;
      }
      QueuePlatformAnimationCommands(next, command_type);
      platform->state = std::move(build_result.platform_animation);
      platform->pending_snapshot = nullptr;
      return true;
    }
    if (has_platform) {
      auto& pending_snapshot = platform->pending_snapshot;
      if (!pending_snapshot) {
        pending_snapshot =
            fml::MakeRefCounted<gfx::PlatformTransitionSnapshot>();
        QueuePlatformAnimationCommands(
            *platform->state, gfx::PlatformAnimationCommandType::kDetach)
            .transition_snapshot = pending_snapshot;
      }
      // The pending snapshot captures the displayed value. The latest update
      // supplies the timing when Core replaces the platform transition.
      platform->state->animation->set_animation_data(data->second);
      return true;
    }
  }
  StartCoreTransition(std::move(build_result.new_animator_animation),
                      play_handles_initial_frame);
  return true;
}

void CSSTransitionManager::StartCoreTransition(
    std::shared_ptr<Animation>&& animation, bool play_handles_initial_frame) {
  animation->SetTransitionFlag();
  animation->NotifyInspectorCreated();
  animation->Play(play_handles_initial_frame);
  animations_map_[animation->name()] = std::move(animation);
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
    case starlight::AnimationPropertyType::kTransform:
    case starlight::AnimationPropertyType::kFilter:
    case starlight::AnimationPropertyType::kBoxShadow:
    case starlight::AnimationPropertyType::kBackgroundPosition:
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

bool CSSTransitionManager::HasActiveTransition(tasm::CSSPropertyID css_id) {
  if (!NeedsTransition(css_id)) {
    return false;
  }
  starlight::AnimationPropertyType property_type =
      GetAnimationPropertyType(css_id);
  const auto& data =
      transition_data_.find(static_cast<unsigned int>(property_type));
  if (data == transition_data_.end()) {
    return false;
  }
  const auto& animation_iterator = animations_map_.find(data->second.name);
  if (animation_iterator == animations_map_.end() ||
      animation_iterator->second == nullptr) {
    return false;
  }
  return animation_iterator->second->GetState() != Animation::State::kStop;
}

bool CSSTransitionManager::ConsumeCSSPropertyForActiveTransition(
    tasm::CSSPropertyID css_id, const tasm::CSSValue& end_value) {
  if (!HasActiveTransition(css_id)) {
    return false;
  }
  auto previous_end_value = previous_end_values_.find(css_id);
  if (previous_end_value != previous_end_values_.end() &&
      previous_end_value->second == end_value) {
    return true;
  }
  return ConsumeCSSProperty(css_id, end_value);
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
