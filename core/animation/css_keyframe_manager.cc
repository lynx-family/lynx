// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/css_keyframe_manager.h"

#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/animation/animation.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/transform_animation_curve.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace animation {

const std::unordered_set<starlight::AnimationPropertyType>&
GetLayoutPropertyTypeSet() {
  static const base::NoDestructor<
      std::unordered_set<starlight::AnimationPropertyType>>
      layoutPropertyTypeSet({ALL_LAYOUT_ANIMATION_PROPERTY});
  return *layoutPropertyTypeSet;
}

const std::unordered_set<AnimationCurve::CurveType>& GetLayoutCurveTypeSet() {
  static const base::NoDestructor<std::unordered_set<AnimationCurve::CurveType>>
      layoutCurveTypeSet({ALL_LAYOUT_CURVE_TYPE});
  return *layoutCurveTypeSet;
}

CSSKeyframeManager::CSSKeyframeManager(tasm::Element* element) {
  element_ = element;
}

KeyframeModel* CSSKeyframeManager::ConstructModel(
    std::unique_ptr<AnimationCurve> curve, AnimationCurve::CurveType type,
    Animation* animation) {
  curve->SetElement(element_);
  // add type here
  curve->type_ = type;
  curve->SetTimingFunction(
      TimingFunction::MakeTimingFunction(animation->animation_data()));
  curve->set_scaled_duration(animation->animation_data()->duration / 1000.0);
  std::unique_ptr<KeyframeModel> new_keyframe_model =
      KeyframeModel::Create(std::move(curve));
  new_keyframe_model->set_animation_data(animation->animation_data());
  KeyframeModel* keyframe_model = new_keyframe_model.get();
  animation->keyframe_effect()->AddKeyframeModel(std::move(new_keyframe_model));
  return keyframe_model;
}

bool CSSKeyframeManager::InitCurveAndModelAndKeyframe(
    AnimationCurve::CurveType type, Animation* animation, double offset,
    std::unique_ptr<TimingFunction> timing_function,
    std::pair<tasm::CSSPropertyID, tasm::CSSValue> css_value_pair) {
  KeyframeModel* keyframe_model =
      animation->keyframe_effect()->GetKeyframeModelByCurveType(type);
  bool has_model = (keyframe_model != nullptr);
  std::unique_ptr<AnimationCurve> new_curve;
  std::unique_ptr<Keyframe> keyframe;
  if (GetLayoutCurveTypeSet().count(type) != 0) {
    if (!has_model) {
      new_curve = KeyframedLayoutAnimationCurve::Create();
    }
    keyframe = LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                      std::move(timing_function));
  } else if (type == AnimationCurve::CurveType::OPACITY) {
    if (!has_model) {
      new_curve = KeyframedOpacityAnimationCurve::Create();
    }
    keyframe = OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                       std::move(timing_function));
  } else if (type == AnimationCurve::CurveType::BGCOLOR ||
             type == AnimationCurve::CurveType::TEXTCOLOR ||
             type == AnimationCurve::CurveType::BORDER_LEFT_COLOR ||
             type == AnimationCurve::CurveType::BORDER_RIGHT_COLOR ||
             type == AnimationCurve::CurveType::BORDER_TOP_COLOR ||
             type == AnimationCurve::CurveType::BORDER_BOTTOM_COLOR) {
    if (!has_model) {
      new_curve = KeyframedColorAnimationCurve::Create(
          element()->computed_css_style()->new_animator_interpolation());
    }
    keyframe = ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                     std::move(timing_function));
  } else if (type == AnimationCurve::CurveType::FLEX_GROW) {
    if (!has_model) {
      new_curve = KeyframedFloatAnimationCurve::Create();
    }
    keyframe = FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                     std::move(timing_function));
  } else if (type == AnimationCurve::CurveType::FILTER) {
    if (!has_model) {
      new_curve = KeyframedFilterAnimationCurve::Create();
    }
    keyframe = FilterKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                      std::move(timing_function));
  } else if (type == AnimationCurve::CurveType::TRANSFORM) {
    if (!has_model) {
      new_curve = KeyframedTransformAnimationCurve::Create();
    }
    keyframe = TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                         std::move(timing_function));
  } else {
    return false;
  }
  // construct keyframe_model with AnimationCurve
  if (!has_model) {
    keyframe_model = ConstructModel(std::move(new_curve), type, animation);
  }
  // set css_value to keyframe
  if (!keyframe.get()->SetValue(css_value_pair, element_)) {
    return false;
  }
  // add keyframe into AnimationCurve
  keyframe_model->animation_curve()->AddKeyframe(std::move(keyframe));
  return true;
}

void CSSKeyframeManager::TickAllAnimation(fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSKeyframeManager::TickAllAnimation");
  auto temp_vec = std::vector<std::weak_ptr<Animation>>();
  auto& true_vec = active_animations_;
  temp_vec.swap(true_vec);
  for (auto& iter : temp_vec) {
    auto animation_shared_ptr = iter.lock();
    if (animation_shared_ptr != nullptr) {
      animation_shared_ptr->DoFrame(frame_time);
    }
  }
  // After traversing the set, the final_animator_maps_ is now assembled.
}

void CSSKeyframeManager::SetAnimationDataAndPlay(
    std::vector<starlight::AnimationData>& anim_data) {
  if (anim_data.size() == animation_data_.size() &&
      std::equal(anim_data.begin(), anim_data.end(), animation_data_.begin())) {
    return;
  }
  animation_data_ = anim_data;
  for (auto& data : animation_data_) {
    if (data.name.empty()) {
      continue;
    }
    // 1. Update data to the existing animation or create a new one, and
    // temporarily save them to temp_active_animations_map_.
    auto animation = animations_map_.find(data.name);
    if (animation != animations_map_.end()) {
      // Update an existing animation, add it to temp_active_animations_map_ and
      // delete it from animations_map_;
      animation->second->UpdateAnimationData(data);
      temp_active_animations_map_[data.name] = animation->second;
      animations_map_.erase(animation);
    } else {
      // Create a new animation, add it to temp_active_animations_map_;
      auto new_animation = CreateAnimation(data);
      temp_active_animations_map_[data.name] = new_animation;
    }
  }
  // 2. All animations remaining in animations_map_ need to be destroyed.
  for (auto& ani_iter : animations_map_) {
    ani_iter.second->Destroy();
  }

  for (auto& active_ani_iter : temp_active_animations_map_) {
    if (active_ani_iter.second->animation_data()->play_state ==
        starlight::AnimationPlayStateType::kPaused) {
      active_ani_iter.second->Pause();
    } else {
      active_ani_iter.second->Play();
    }
  }
  // 3. Swap active animations to animations_map_.
  animations_map_.swap(temp_active_animations_map_);
  temp_active_animations_map_.clear();
}

std::shared_ptr<Animation> CSSKeyframeManager::CreateAnimation(
    starlight::AnimationData& data) {
  // 1. create animation & keyframe_effect according to animation data
  auto animation = std::make_shared<Animation>(data.name.str());
  animation->set_animation_data(data);

  std::unique_ptr<KeyframeEffect> keyframe_effect = KeyframeEffect::Create();
  keyframe_effect->BindAnimationDelegate(this);
  keyframe_effect->BindElement(this->element());
  animation->SetKeyframeEffect(std::move(keyframe_effect));
  animation->BindDelegate(this);
  animation->BindElement(this->element());
  // 2. create keyframe Models& animation Curves according to CSS keyframe
  // tokens
  MakeKeyframeModel(animation.get(), data.name.str());
  LOGI("Animation create complete, name is: " << data.name.str());
  return animation;
}

tasm::CSSKeyframesContent& CSSKeyframeManager::GetKeyframesStyleMap(
    const std::string& animation_name) {
  DCHECK(element() != nullptr);
  auto iter = element()->keyframes_map().find(animation_name);
  if (iter != element()->keyframes_map().end()) {
    return iter->second->GetKeyframesContent();
  }
  tasm::CSSKeyframesToken* tokens =
      element()->GetCSSKeyframesToken(animation_name);
  if (tokens) {
    return tokens->GetKeyframesContent();
  }
  return empty_keyframe_map();
}

void CSSKeyframeManager::MakeKeyframeModel(Animation* animation,
                                           const std::string& animation_name) {
  const auto& keyframes_map = GetKeyframesStyleMap(animation_name);
  for (const auto& keyframe_info : keyframes_map) {
    double offset = keyframe_info.first;
    tasm::StyleMap* style_map = keyframe_info.second.get();
    if (!style_map) {
      continue;
    }
    std::unique_ptr<TimingFunction> timing_function = nullptr;
    starlight::TimingFunctionData timing_function_for_keyframe;
    const auto& iter =
        style_map->find(tasm::kPropertyIDAnimationTimingFunction);
    if (iter != style_map->end()) {
      auto timing_function_value =
          iter->second.GetValue().Array().get()->get(0);
      starlight::CSSStyleUtils::ComputeTimingFunction(
          timing_function_value, false, timing_function_for_keyframe,
          element_->element_manager()->GetCSSParserConfigs());
    }
    for (const auto& css_value_pair : *style_map) {
      if (css_value_pair.first == tasm::kPropertyIDAnimationTimingFunction) {
        continue;
      }
      timing_function =
          TimingFunction::MakeTimingFunction(timing_function_for_keyframe);
      AnimationCurve::CurveType curve_type =
          static_cast<AnimationCurve::CurveType>(css_value_pair.first);
      if (GetPropertyIDToAnimationPropertyTypeMap().find(
              css_value_pair.first) ==
          GetPropertyIDToAnimationPropertyTypeMap().end()) {
        LOGE("[animation] unsupported animation curve type for css:"
             << css_value_pair.first);
        continue;
      }
      bool init_status = InitCurveAndModelAndKeyframe(
          curve_type, animation, offset, std::move(timing_function),
          css_value_pair);
      if (!init_status) {
        continue;
      }
      animation->SetRawCssId(css_value_pair.first);
    }
  }
  // There may be no from(0%) and to(100%) keyframe. If so, we add a empty one.
  animation->keyframe_effect()->EnsureFromAndToKeyframe();
}

void CSSKeyframeManager::RequestNextFrame(std::weak_ptr<Animation> ptr) {
  active_animations_.push_back(ptr);
  element_->RequestNextFrame();
}

void CSSKeyframeManager::UpdateFinalStyleMap(const tasm::StyleMap& styles) {
  element()->UpdateFinalStyleMap(styles);
}

void CSSKeyframeManager::NotifyClientAnimated(tasm::StyleMap& styles,
                                              tasm::CSSValue value,
                                              tasm::CSSPropertyID css_id) {
  if (!element_) {
    return;
  }
  switch (css_id) {
    case tasm::kPropertyIDTransform: {
      // If the transform value is empty, we use transform default value to fit
      // the CSS parsing logic.
      if (value.IsEmpty() ||
          (value.IsArray() && value.GetValue().Array()->size() == 0)) {
        value = GetDefaultValue(starlight::AnimationPropertyType::kTransform);
      }
      break;
    }
    case tasm::kPropertyIDOpacity: {
      if (value.IsNumber() && value.GetValue().Number() < 0.0f) {
        return;
      }
      break;
    }
    default: {
      break;
    }
  }
  if (styles.find(css_id) != styles.end()) {
    styles.erase(css_id);
  }
  styles.insert_or_assign(css_id, std::move(value));
}

void CSSKeyframeManager::SetNeedsAnimationStyleRecalc(const std::string& name) {
  // clear effect
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "CSSKeyframeManager::SetNeedsAnimationStyleRecalc");
  if (element_) {
    auto iter = animations_map_.find(name);
    if (iter == animations_map_.end()) {
      iter = temp_active_animations_map_.find(name);
      if (iter == temp_active_animations_map_.end()) {
        return;
      }
    }
    auto animation = iter->second;
    if (animation) {
      tasm::StyleMap reset_origin_css_styles;
      const auto& raw_style_set = animation->GetRawStyleSet();
      reset_origin_css_styles.reserve(raw_style_set.size());
      for (tasm::CSSPropertyID key : raw_style_set) {
        std::optional<tasm::CSSValue> value_opt =
            element_->GetElementStyle(key);
        if (!value_opt) {
          reset_origin_css_styles[key] = tasm::CSSValue::Empty();
        } else {
          reset_origin_css_styles[key] = std::move(*value_opt);
        }
      }
      element()->UpdateFinalStyleMap(reset_origin_css_styles);
    }
  }
}

void CSSKeyframeManager::FlushAnimatedStyle() {
  element()->FlushAnimatedStyle();
}

const tasm::CssMeasureContext& CSSKeyframeManager::GetLengthContext(
    tasm::Element* element) {
  if (!element || !element->computed_css_style()) {
    static base::NoDestructor<tasm::CssMeasureContext> sDefaultLengthContext(
        0.f,
        element->computed_css_style()->GetMeasureContext().layouts_unit_per_px_,
        element->computed_css_style()
            ->GetMeasureContext()
            .physical_pixels_per_layout_unit_,
        element->computed_css_style()
                ->GetMeasureContext()
                .layouts_unit_per_px_ *
            DEFAULT_FONT_SIZE_DP,
        element->computed_css_style()
                ->GetMeasureContext()
                .layouts_unit_per_px_ *
            DEFAULT_FONT_SIZE_DP,
        starlight::LayoutUnit(), starlight::LayoutUnit());
    return *sDefaultLengthContext;
  }
  return element->computed_css_style()->GetMeasureContext();
}

tasm::CSSValue CSSKeyframeManager::GetDefaultValue(
    starlight::AnimationPropertyType type) {
  if (GetLayoutPropertyTypeSet().count(type) != 0) {
    // the default values of layout properties are 'auto'.
    return tasm::CSSValue::Empty();
  } else if (type == starlight::AnimationPropertyType::kOpacity) {
    return tasm::CSSValue(lepus_value(OpacityKeyframe::kDefaultOpacity),
                          tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kBackgroundColor ||
             (type >= starlight::AnimationPropertyType::kBorderTopColor &&
              type <= starlight::AnimationPropertyType::kBorderBottomColor)) {
    return tasm::CSSValue(lepus_value(ColorKeyframe::kDefaultBackgroundColor),
                          tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kColor) {
    return tasm::CSSValue(lepus_value(ColorKeyframe::kDefaultTextColor),
                          tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kTransform) {
    // There are many kinds of identity transforms, we choose one(rotateZ 0
    // degree) of them.
    auto items = lepus::CArray::Create();
    auto item = lepus::CArray::Create();
    item->emplace_back(static_cast<int>(starlight::TransformType::kRotateZ));
    item->emplace_back(0.0f);
    items->emplace_back(std::move(item));
    return tasm::CSSValue(std::move(items));
  } else if (type == starlight::AnimationPropertyType::kFlexGrow) {
    return tasm::CSSValue(lepus_value(FloatKeyframe::kDefaultFloatValue),
                          tasm::CSSValuePattern::NUMBER);
  }
  return tasm::CSSValue::Empty();
}

// TODO:(wujintian) Remove AnimationPropertyType, it is redundant code. Only use
// AnimationCurve::CurveType and tasm::kPropertyIDxxx in animation code.
const std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>&
GetPropertyIDToAnimationPropertyTypeMap() {
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyMap({
#define DECLARE_ID_TYPE_MAP(id, type) \
  {tasm::id, starlight::AnimationPropertyType::type},
          FOREACH_NEW_ANIMATOR_PROPERTY(DECLARE_ID_TYPE_MAP)
#undef DECLARE_ID_TYPE_MAP
      });
  return *kIDPropertyMap;
}

const std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>&
GetPolymericPropertyIDToAnimationPropertyTypeMap(
    starlight::AnimationPropertyType polymeric_type) {
  if (polymeric_type == starlight::AnimationPropertyType::kBorderWidth) {
    static const base::NoDestructor<std::unordered_map<
        tasm::CSSPropertyID, starlight::AnimationPropertyType>>
        kIDPropertyBorderWidthMap({
            {tasm::kPropertyIDBorderTopWidth,
             starlight::AnimationPropertyType::kBorderTopWidth},
            {tasm::kPropertyIDBorderLeftWidth,
             starlight::AnimationPropertyType::kBorderLeftWidth},
            {tasm::kPropertyIDBorderRightWidth,
             starlight::AnimationPropertyType::kBorderRightWidth},
            {tasm::kPropertyIDBorderBottomWidth,
             starlight::AnimationPropertyType::kBorderBottomWidth},
        });
    return *kIDPropertyBorderWidthMap;
  } else if (polymeric_type == starlight::AnimationPropertyType::kBorderColor) {
    static const base::NoDestructor<std::unordered_map<
        tasm::CSSPropertyID, starlight::AnimationPropertyType>>
        kIDPropertyBorderColorMap({
            {tasm::kPropertyIDBorderTopColor,
             starlight::AnimationPropertyType::kBorderTopColor},
            {tasm::kPropertyIDBorderLeftColor,
             starlight::AnimationPropertyType::kBorderLeftColor},
            {tasm::kPropertyIDBorderRightColor,
             starlight::AnimationPropertyType::kBorderRightColor},
            {tasm::kPropertyIDBorderBottomColor,
             starlight::AnimationPropertyType::kBorderBottomColor},
        });
    return *kIDPropertyBorderColorMap;
  } else if (polymeric_type == starlight::AnimationPropertyType::kMargin) {
    static const base::NoDestructor<std::unordered_map<
        tasm::CSSPropertyID, starlight::AnimationPropertyType>>
        kIDPropertyMarginMap({
            {tasm::kPropertyIDMarginTop,
             starlight::AnimationPropertyType::kMarginTop},
            {tasm::kPropertyIDMarginLeft,
             starlight::AnimationPropertyType::kMarginLeft},
            {tasm::kPropertyIDMarginRight,
             starlight::AnimationPropertyType::kMarginRight},
            {tasm::kPropertyIDMarginBottom,
             starlight::AnimationPropertyType::kMarginBottom},
        });
    return *kIDPropertyMarginMap;
  } else if (polymeric_type == starlight::AnimationPropertyType::kPadding) {
    static const base::NoDestructor<std::unordered_map<
        tasm::CSSPropertyID, starlight::AnimationPropertyType>>
        kIDPropertyPaddingMap({
            {tasm::kPropertyIDPaddingTop,
             starlight::AnimationPropertyType::kPaddingTop},
            {tasm::kPropertyIDPaddingLeft,
             starlight::AnimationPropertyType::kPaddingLeft},
            {tasm::kPropertyIDPaddingRight,
             starlight::AnimationPropertyType::kPaddingRight},
            {tasm::kPropertyIDPaddingBottom,
             starlight::AnimationPropertyType::kPaddingBottom},
        });
    return *kIDPropertyPaddingMap;
  } else {
    static const base::NoDestructor<std::unordered_map<
        tasm::CSSPropertyID, starlight::AnimationPropertyType>>
        kIDPropertyMap({});
    return *kIDPropertyMap;
  }
}

void CSSKeyframeManager::NotifyElementSizeUpdated() {
  for (auto& item : animations_map_) {
    item.second->NotifyElementSizeUpdated();
  }
}

void CSSKeyframeManager::NotifyUnitValuesUpdatedToAnimation(
    tasm::CSSValuePattern type) {
  for (auto& item : animations_map_) {
    item.second->NotifyUnitValuesUpdatedToAnimation(type);
  }
}

const std::unordered_set<tasm::CSSPropertyID>& GetAnimatablePropertyIDSet() {
  static const base::NoDestructor<std::unordered_set<tasm::CSSPropertyID>>
      animatablePropertyIDSet({ALL_ANIMATABLE_PROPERTY_ID});
  return *animatablePropertyIDSet;
}

bool IsAnimatableProperty(tasm::CSSPropertyID css_id) {
  return GetAnimatablePropertyIDSet().count(css_id) != 0;
}

}  // namespace animation
}  // namespace lynx
