// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/css_keyframe_manager.h"

#include <algorithm>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "base/include/flex_optional.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "core/animation/animation.h"
#include "core/animation/animation_backend_evaluator.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/animation_trace_event_def.h"
#include "core/animation/constants.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/transform_animation_curve.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/style/css_type.h"
#include "gfx/animation/timing_function.h"

namespace lynx {
namespace animation {

namespace {

template <typename Keyframe>
bool SetCSSKeyframeValue(Keyframe* keyframe, tasm::CSSPropertyID id,
                         const tasm::CSSValue& value, tasm::Element* element) {
  return keyframe->SetValue(id, value, element);
}

template <typename Keyframe>
bool SetNumericCSSKeyframeValue(Keyframe* keyframe, tasm::CSSPropertyID id,
                                const tasm::CSSValue& value,
                                tasm::Element* element) {
  auto resolved_value = HandleCSSVariableValueIfNeed(id, value, element);
  if (!resolved_value.IsNumber()) {
    return false;
  }
  keyframe->SetValue(
      static_cast<decltype(keyframe->Value())>(resolved_value.GetNumber()));
  return true;
}

bool SetCSSKeyframeValue(gfx::FloatKeyframe* keyframe, tasm::CSSPropertyID id,
                         const tasm::CSSValue& value, tasm::Element* element) {
  return SetNumericCSSKeyframeValue(keyframe, id, value, element);
}

bool SetCSSKeyframeValue(gfx::ColorKeyframe* keyframe, tasm::CSSPropertyID id,
                         const tasm::CSSValue& value, tasm::Element* element) {
  return SetNumericCSSKeyframeValue(keyframe, id, value, element);
}

bool SetCSSKeyframeValue(gfx::IntKeyframe* keyframe, tasm::CSSPropertyID id,
                         const tasm::CSSValue& value, tasm::Element* element) {
  auto resolved_value = HandleCSSVariableValueIfNeed(id, value, element);
  if (!resolved_value.IsEnum()) {
    return false;
  }
  keyframe->SetValue(static_cast<int32_t>(resolved_value.AsNumber()));
  return true;
}

bool HasNoSampleableKeyframes(const std::shared_ptr<Animation>& animation,
                              bool has_custom_property_keyframes) {
  if (animation == nullptr || animation->keyframe_effect() == nullptr) {
    return true;
  }
  return animation->keyframe_effect()->keyframe_models().empty() &&
         !has_custom_property_keyframes;
}

void SyncAnimationRawCustomPropertySet(
    Animation* animation,
    const tasm::CSSKeyframesCustomPropertyContent& custom_property_content) {
  if (animation == nullptr) {
    return;
  }
  animation->ClearRawCustomProperties();
  for (const auto& keyframe_custom_property : custom_property_content) {
    const auto& custom_properties = keyframe_custom_property.second;
    if (custom_properties == nullptr) {
      continue;
    }
    for (const auto& custom_property : *custom_properties) {
      animation->SetRawCustomProperty(custom_property.first);
    }
  }
}

}  // namespace

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

namespace {

base::flex_optional<tasm::CSSValue> GetUnderlyingValueFromComputedStyle(
    tasm::CSSPropertyID id, const starlight::ComputedCSSStyle& computed_style,
    const tasm::StyleMap* underlying_layout_only_styles) {
  if (underlying_layout_only_styles != nullptr) {
    auto iter = underlying_layout_only_styles->find(id);
    if (iter != underlying_layout_only_styles->end()) {
      return iter->second;
    }
  }

  const auto& resolved_values = computed_style.GetResolvedValues();
  auto resolved_iter = resolved_values.find(id);
  if (resolved_iter != resolved_values.end()) {
    return resolved_iter->second;
  }

  auto canonical_value = computed_style.ExtractCanonicalComputedValue(id);
  if (!canonical_value.has_value()) {
    return {};
  }
  return ConvertCanonicalComputedValueForAnimation(
      id, *canonical_value, computed_style.GetMeasureContext());
}

}  // namespace

KeyframeModel* CSSKeyframeManager::ConstructModel(
    std::unique_ptr<AnimationCurve> curve, AnimationCurve::CurveType type,
    Animation* animation) {
  curve->SetElement(element_);
  curve->type_ = type;
  std::unique_ptr<KeyframeModel> new_keyframe_model =
      KeyframeModel::Create(std::move(curve));
  new_keyframe_model->UpdateAnimationData(&animation->get_animation_data());
  KeyframeModel* keyframe_model = new_keyframe_model.get();
  animation->keyframe_effect()->AddKeyframeModel(std::move(new_keyframe_model));
  return keyframe_model;
}

CSSKeyframeManager::ParsedKeyframe CSSKeyframeManager::CreateParsedKeyframe(
    AnimationCurve::CurveType type, double offset,
    std::unique_ptr<gfx::TimingFunction> timing_function,
    tasm::CSSPropertyID id, const tasm::CSSValue* value) {
  ParsedKeyframe result;
  const auto time = fml::TimeDelta::FromSecondsF(offset);
  auto init_keyframe = [&](auto typed_keyframe) {
    if (value != nullptr &&
        !SetCSSKeyframeValue(typed_keyframe.get(), id, *value, element_)) {
      return;
    }
    result.callbacks = MakeKeyframeCallbacks(typed_keyframe.get());
    typed_keyframe->SetTimingSource(gfx::Keyframe::TimingSource::kAnimation);
    result.keyframe = std::move(typed_keyframe);
  };

  if (GetLayoutCurveTypeSet().count(type) != 0) {
    init_keyframe(LayoutKeyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::OPACITY ||
             type == AnimationCurve::CurveType::FLEX_GROW ||
             type == AnimationCurve::CurveType::OFFSET_DISTANCE) {
    init_keyframe(gfx::FloatKeyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::BGCOLOR ||
             type == AnimationCurve::CurveType::TEXTCOLOR ||
             type == AnimationCurve::CurveType::BORDER_LEFT_COLOR ||
             type == AnimationCurve::CurveType::BORDER_RIGHT_COLOR ||
             type == AnimationCurve::CurveType::BORDER_TOP_COLOR ||
             type == AnimationCurve::CurveType::BORDER_BOTTOM_COLOR) {
    init_keyframe(gfx::ColorKeyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::FILTER) {
    init_keyframe(FilterKeyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::BOX_SHADOW) {
    init_keyframe(BoxShadowKeyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::TRANSFORM) {
    init_keyframe(TransformKeyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::BACKGROUND_POSITION ||
             type == AnimationCurve::CurveType::TRANSFORM_ORIGIN) {
    init_keyframe(CSSVec2Keyframe::Create(time, std::move(timing_function)));
  } else if (type == AnimationCurve::CurveType::VISIBILITY) {
    init_keyframe(gfx::IntKeyframe::Create(time, std::move(timing_function)));
  }
  return result;
}

std::unique_ptr<AnimationCurve> CSSKeyframeManager::CreateCurve(
    AnimationCurve::CurveType type) {
  if (GetLayoutCurveTypeSet().count(type) != 0) {
    return KeyframedLayoutAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::OPACITY) {
    return KeyframedOpacityAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::BGCOLOR ||
      type == AnimationCurve::CurveType::TEXTCOLOR ||
      type == AnimationCurve::CurveType::BORDER_LEFT_COLOR ||
      type == AnimationCurve::CurveType::BORDER_RIGHT_COLOR ||
      type == AnimationCurve::CurveType::BORDER_TOP_COLOR ||
      type == AnimationCurve::CurveType::BORDER_BOTTOM_COLOR) {
    return KeyframedColorAnimationCurve::Create(
        element()->computed_css_style()->new_animator_interpolation());
  }
  if (type == AnimationCurve::CurveType::FLEX_GROW ||
      type == AnimationCurve::CurveType::OFFSET_DISTANCE) {
    return KeyframedFloatAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::FILTER) {
    return KeyframedFilterAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::BOX_SHADOW) {
    return KeyframedBoxShadowAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::TRANSFORM) {
    return KeyframedTransformAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::BACKGROUND_POSITION) {
    return KeyframedBackgroundPositionAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::TRANSFORM_ORIGIN) {
    return KeyframedTransformOriginAnimationCurve::Create();
  }
  if (type == AnimationCurve::CurveType::VISIBILITY) {
    return KeyframedVisibilityAnimationCurve::Create();
  }
  return nullptr;
}

void CSSKeyframeManager::AddParsedPropertyToNewAnimator(
    ParsedPropertyKeyframes&& property, Animation* animation) {
  auto curve = CreateCurve(property.curve_type);
  if (!curve) {
    return;
  }
  // Neutral endpoints retain no concrete value. Sampling resolves them using
  // this underlying snapshot and the current element context.
  curve->SetUnderlyingValue(std::move(property.underlying_value));
  for (auto& parsed : property.keyframes) {
    curve->AddKeyframe(std::move(parsed.keyframe), parsed.callbacks);
  }
  ConstructModel(std::move(curve), property.curve_type, animation);
}

fml::TimePoint CSSKeyframeManager::ProcessAnimationEvents(
    fml::TimePoint& frame_time, bool dispatch_events,
    bool suppress_infinite_events) {
  auto next = fml::TimePoint::Max();
  // Retain animations during event dispatch, keeping the frame queue intact.
  base::InlineVector<std::shared_ptr<Animation>, 4> animations;
  animations.reserve(animations_map_.size());
  for (const auto& entry : animations_map_) {
    if (entry.second) {
      animations.push_back(entry.second);
    }
  }
  for (const auto& animation : animations) {
    if (suppress_infinite_events &&
        animation->GetOrigin() == Animation::Origin::kCSSAnimation &&
        animation->IsInfinite()) {
      continue;
    }
    auto time = animation->GetNextEventTime(frame_time);
    if (dispatch_events && time <= frame_time) {
      animation->TickEvents(frame_time);
      time = animation->GetNextEventTime(frame_time);
    }
    next = std::min(next, time);
  }
  return next;
}

AnimationBackendResult CSSKeyframeManager::EvaluatePlatformSupport(
    ParsedPropertyMap& properties) {
  // TODO: Profile caching eligibility by kind/property, keyframes/timing,
  // dynamic inputs, events and capabilities. Hits must still resolve endpoints
  // and validate the node.
  auto* painting_context = element_->painting_context();
  if (painting_context == nullptr) {
    return {false, AnimationFallbackReason::kBackendUnavailable};
  }

  const auto& capabilities =
      painting_context->GetPlatformAnimationCapabilities();
  gfx::AnimationEventCapabilityMask required_events = 0;
  auto require_if_bound = [&](const char* name,
                              gfx::AnimationEventCapabilityMask event) {
    const base::String key(name);
    if (element_->event_map().count(key) ||
        element_->lepus_event_map().count(key) ||
        element_->global_bind_event_map().count(key)) {
      required_events |= event;
    }
  };
  const bool keyframe = GetAnimationKind() == gfx::AnimationKind::kKeyframe;
  require_if_bound(
      keyframe ? kKeyframeStartEventName : kTransitionStartEventName,
      gfx::kAnimationEventStart);
  require_if_bound(keyframe ? kKeyframeEndEventName : kTransitionEndEventName,
                   gfx::kAnimationEventEnd);
  require_if_bound(
      keyframe ? kKeyframeCancelEventName : kTransitionCancelEventName,
      gfx::kAnimationEventCancel);
  if (keyframe) {
    require_if_bound(kKeyframeIterationEventName,
                     gfx::kAnimationEventIteration);
  }

  AnimationBackendResult effect_result;
  for (auto& entry : properties) {
    auto& property = entry.second;
    AnimationBackendRequest request;
    request.kind = GetAnimationKind();
    request.required_events = required_events;
    request.property = property.gfx_property;
    request.has_dynamic_dependencies = property.has_dynamic_dependencies;
    request.keyframes.reserve(property.keyframes.size());
    for (auto& parsed : property.keyframes) {
      if (parsed.keyframe == nullptr) {
        return {false, AnimationFallbackReason::kUnresolvedKeyframe};
      }
      if (parsed.keyframe->IsEmpty()) {
        const auto& value = property.underlying_value;
        if (value.IsEmpty()) {
          return {false, AnimationFallbackReason::kUnresolvedKeyframe};
        }
        auto resolved = CreateParsedKeyframe(
            property.curve_type, parsed.keyframe->Offset(),
            parsed.keyframe->timing_function()
                ? parsed.keyframe->timing_function()->Clone()
                : nullptr,
            property.css_id, &value);
        if (!resolved.keyframe || resolved.keyframe->IsEmpty()) {
          return {false, AnimationFallbackReason::kUnresolvedKeyframe};
        }
        resolved.keyframe->SetTimingSource(parsed.keyframe->timing_source());
        request.has_dynamic_dependencies |= value.IsVariable();
        if (resolved.keyframe->ValueType() ==
            gfx::KeyframeValueType::kTransform) {
          request.has_dynamic_dependencies |=
              static_cast<const TransformKeyframe*>(resolved.keyframe.get())
                  ->HasDynamicDependencies();
        }
        parsed.resolved_platform_endpoint = std::move(resolved.keyframe);
      }
      request.keyframes.push_back(parsed.KeyframeForPlatform());
    }
    const auto result = EvaluateAnimationBackend(request, capabilities);
    if (!result.CanRun()) {
      return result;
    }
    // All properties in one platform effect must share its timing function.
    if (effect_result.CanRun() &&
        !(effect_result.platform_timing == result.platform_timing)) {
      return {false, AnimationFallbackReason::kUnsupportedTimingFunction};
    }
    effect_result = result;
  }
  return effect_result;
}

void CSSKeyframeManager::SyncInfiniteAnimationEvents(
    fml::TimePoint& frame_time) {
  base::InlineVector<std::shared_ptr<Animation>, 4> animations;
  animations.reserve(animations_map_.size());
  for (const auto& entry : animations_map_) {
    if (entry.second) {
      animations.push_back(entry.second);
    }
  }
  for (const auto& animation : animations) {
    if (animation->GetOrigin() == Animation::Origin::kCSSAnimation &&
        animation->IsInfinite()) {
      // Advance event bookkeeping without dispatching events. This prevents
      // all iterations crossed while paused from being replayed at resume.
      animation->SampleAt(frame_time, true);
    }
  }
}

void CSSKeyframeManager::TickAllAnimation(fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_MANAGER_TICK_ALL_ANIMATION);
  base::InlineVector<std::shared_ptr<Animation>, 4> animations;
  // Snapshot and clear requests before DoFrame(), so new requests are kept
  // for the next frame. Keyframes already follow CSS declaration order.
  for (const auto& [name, animation] : animations_map_) {
    if (animation &&
        std::exchange(animation->has_pending_frame_request_, false)) {
      animations.push_back(animation);
    }
  }
  for (const auto& animation : animations) {
    animation->DoFrame(frame_time);
  }
  // After traversing the set, the final_animator_maps_ is now assembled.
}

void CSSKeyframeManager::SetAnimationDataAndPlay(
    base::Vector<starlight::AnimationData>& anim_data) {
  // Legacy styling relies on Play/Pause to sample and flush initial styles.
  SetAnimationDataAndPlayInternal(anim_data, false, true, false);
}

void CSSKeyframeManager::SetAnimationDataAndPlayInternal(
    base::Vector<starlight::AnimationData>& anim_data, bool force_rebuild,
    bool play_handles_initial_frame, bool use_new_pipeline_cleanup,
    const tasm::StyleMap* new_base_resolved_styles,
    const tasm::StyleMap* new_underlying_layout_only_styles,
    const tasm::CustomPropertiesMap* new_base_custom_properties) {
  if (anim_data.size() == animation_data_.size() &&
      std::equal(anim_data.begin(), anim_data.end(), animation_data_.begin()) &&
      !force_rebuild) {
    return;
  }
  animation_data_ = anim_data;
  for (auto& data : animation_data_) {
    if (data.name.empty()) {
      continue;
    }
    // negative duration is invalid, set it to 0.
    if (data.duration < 0) {
      data.duration = 0;
    }
    const bool has_custom_property_keyframes =
        starlight::CSSStyleUtils::HasNonEmptyCSSKeyframesCustomPropertyContent(
            GetKeyframesCustomPropertyMap(data.name));
    // 1. Update data to the existing animation or create a new one, and
    // temporarily save them to temp_active_animations_map_.
    auto animation = animations_map_.find(data.name);
    if (animation != animations_map_.end()) {
      // Update an existing animation, add it to temp_active_animations_map_ and
      // delete it from animations_map_;
      if (force_rebuild) {
        auto replacement =
            CreateAnimation(data, new_base_custom_properties,
                            animation->second->GetOrigin(), false);
        if (replacement) {
          if (use_new_pipeline_cleanup) {
            ClearAnimationEffects(animation->second, new_base_resolved_styles,
                                  new_underlying_layout_only_styles);
          } else {
            SetNeedsAnimationStyleRecalc(data.name);
            FlushAnimatedStyle();
          }
          animation->second->ReplaceEffectFrom(*replacement);
          temp_active_animations_map_[data.name] = animation->second;
        } else if (use_new_pipeline_cleanup) {
          PrepareAnimationRemoval(animation->second, new_base_resolved_styles,
                                  new_underlying_layout_only_styles);
        } else {
          animation->second->Destroy();
        }
        animations_map_.erase(animation);
        continue;
      }
      animation->second->keyframe_effect()->SetHasCustomPropertyKeyframes(
          has_custom_property_keyframes);
      SyncAnimationRawCustomPropertySet(
          animation->second.get(), GetKeyframesCustomPropertyMap(data.name));
      if (animation->second->GetState() == Animation::State::kStop &&
          HasNoSampleableKeyframes(animation->second,
                                   has_custom_property_keyframes)) {
        const auto origin = animation->second->GetOrigin();
        if (use_new_pipeline_cleanup) {
          PrepareAnimationRemoval(animation->second, new_base_resolved_styles,
                                  new_underlying_layout_only_styles);
        } else {
          animation->second->Destroy();
        }
        auto recreated_animation =
            CreateAnimation(data, new_base_custom_properties, origin);
        if (recreated_animation != nullptr) {
          temp_active_animations_map_[data.name] = recreated_animation;
        }
        animations_map_.erase(animation);
        continue;
      }
      if (animation->second->get_animation_data() != data) {
        if (use_new_pipeline_cleanup &&
            animation->second->GetState() == Animation::State::kStop) {
          ClearAnimationEffects(animation->second, new_base_resolved_styles,
                                new_underlying_layout_only_styles);
        }
        animation->second->UpdateAnimationData(data);
        temp_active_animations_map_[data.name] = animation->second;
      } else {
        temp_keep_animations_map_[data.name] = animation->second;
      }
      animations_map_.erase(animation);
    } else {
      // Create a new animation, add it to temp_active_animations_map_;
      auto new_animation = CreateAnimation(data, new_base_custom_properties);
      if (new_animation != nullptr) {
        temp_active_animations_map_[data.name] = new_animation;
      }
    }
  }
  //   2. All animations remaining in animations_map_ need to be destroyed.
  for (auto& ani_iter : animations_map_) {
    if (use_new_pipeline_cleanup) {
      PrepareAnimationRemoval(ani_iter.second, new_base_resolved_styles,
                              new_underlying_layout_only_styles);
    } else {
      ani_iter.second->Destroy();
    }
  }

  for (auto& active_ani_iter : temp_active_animations_map_) {
    if (active_ani_iter.second->animation_data()->play_state ==
        starlight::AnimationPlayStateType::kPaused) {
      active_ani_iter.second->Pause(play_handles_initial_frame);
    } else {
      active_ani_iter.second->Play(play_handles_initial_frame);
    }
  }
  // Keep changed and unchanged animations interleaved in CSS declaration order.
  temp_active_animations_map_.merge(temp_keep_animations_map_);
  animations_map_.clear();
  for (const auto& data : animation_data_) {
    auto animation = temp_active_animations_map_.find(data.name);
    if (animation != temp_active_animations_map_.end()) {
      animations_map_.insert(*animation);
    }
  }
  temp_keep_animations_map_.clear();
  temp_active_animations_map_.clear();
}

void CSSKeyframeManager::SyncAnimationDataForNewPipeline(
    base::Vector<starlight::AnimationData>& anim_data, bool force_rebuild,
    const tasm::StyleMap* new_base_resolved_styles,
    const tasm::StyleMap* new_underlying_layout_only_styles,
    const tasm::CustomPropertiesMap* new_base_custom_properties,
    const starlight::ComputedCSSStyle* new_base_style) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              KEYFRAME_MANAGER_SYNC_ANIMATION_DATA_FOR_NEW_PIPELINE);
  // New styling samples in CollectAnimationUpdatesForNewPipeline() and
  // composes the final style in the same outer style pass.
  SetAnimationDataAndPlayInternal(
      anim_data, force_rebuild, false, true, new_base_resolved_styles,
      new_underlying_layout_only_styles, new_base_custom_properties);
  if (new_base_style != nullptr) {
    UpdateUnderlyingValuesFromComputedStyle(*new_base_style,
                                            new_underlying_layout_only_styles);
  }
}

void CSSKeyframeManager::UpdateUnderlyingValue(tasm::CSSPropertyID id,
                                               const tasm::CSSValue& value) {
  const auto curve_type = static_cast<AnimationCurve::CurveType>(id);
  for (auto& animation_iter : animations_map_) {
    animation_iter.second->UpdateUnderlyingValue(curve_type, value);
  }
}

void CSSKeyframeManager::UpdateUnderlyingValueFromComputedStyle(
    tasm::CSSPropertyID id, const starlight::ComputedCSSStyle& computed_style) {
  auto value = GetUnderlyingValueFromComputedStyle(id, computed_style, nullptr);
  if (value.has_value()) {
    UpdateUnderlyingValue(id, *value);
  }
}

void CSSKeyframeManager::UpdateUnderlyingValuesFromComputedStyle(
    const starlight::ComputedCSSStyle& computed_style,
    const tasm::StyleMap* underlying_layout_only_styles) {
  for (auto& animation_iter : animations_map_) {
    for (auto& model :
         animation_iter.second->keyframe_effect()->keyframe_models()) {
      auto* curve = model->animation_curve();
      auto value = GetUnderlyingValueFromComputedStyle(
          static_cast<tasm::CSSPropertyID>(curve->Type()), computed_style,
          underlying_layout_only_styles);
      if (value.has_value()) {
        animation_iter.second->UpdateUnderlyingValue(curve->Type(), *value);
      }
    }
  }
}

AnimationSampleForNewPipeline
CSSKeyframeManager::CollectAnimationUpdatesForNewPipeline(
    fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              KEYFRAME_MANAGER_COLLECT_ANIMATION_UPDATES_FOR_NEW_PIPELINE);
  if (animations_map_.empty() && pending_property_overrides_.empty() &&
      pending_property_resets_.empty() &&
      pending_custom_property_resets_.empty() &&
      persisted_property_fill_styles_.empty() &&
      persisted_custom_property_fill_styles_.empty()) {
    return {};
  }

  base::flex_optional<AnimationSampleForNewPipeline> sample;
  auto ensure_sample = [&sample]() -> AnimationSampleForNewPipeline& {
    if (!sample.has_value()) {
      sample.emplace();
    }
    return *sample;
  };
  {
    // DrainPendingAnimationCleanup
    if (!pending_property_overrides_.empty()) {
      ensure_sample().property_overrides =
          std::move(pending_property_overrides_);
      pending_property_overrides_.clear();
    }
    if (!pending_property_resets_.empty()) {
      auto& sample_ref = ensure_sample();
      sample_ref.property_resets = std::move(pending_property_resets_);
      sample_ref.requires_base_style_rebuild = true;
      pending_property_resets_.clear();
    }
    if (!pending_custom_property_resets_.empty()) {
      auto& sample_ref = ensure_sample();
      sample_ref.custom_property_resets =
          std::move(pending_custom_property_resets_);
      sample_ref.requires_base_style_rebuild = true;
      pending_custom_property_resets_.clear();
    }
    if (!persisted_property_fill_styles_.empty()) {
      auto& sample_ref = ensure_sample();
      for (const auto& [key, value] : persisted_property_fill_styles_) {
        sample_ref.property_overrides.insert_or_assign(key, value);
      }
    }
    if (!persisted_custom_property_fill_styles_.empty()) {
      auto& sample_ref = ensure_sample();
      for (const auto& [key, value] : persisted_custom_property_fill_styles_) {
        sample_ref.custom_property_overrides.insert_or_assign(key, value);
      }
      sample_ref.requires_base_style_rebuild = true;
    }
  }

  {
    // SampleRunningAnimations
    base::InlineVector<std::shared_ptr<Animation>, 4> animations;
    for (const auto& item : animations_map_) {
      animations.push_back(item.second);
    }
    for (const auto& animation : animations) {
      if (animation == nullptr || animation->keyframe_effect() == nullptr) {
        continue;
      }
      // SampleAt samples normal CSS property curves into styles and advances
      // timeline side effects. For custom-property-only keyframes it still
      // synthesizes start/end/iteration events even though styles stays empty.
      auto sample_result = animation->SampleAt(frame_time);
      if (!sample_result.styles.empty()) {
        auto& sample_ref = ensure_sample();
        for (const auto& [key, value] : sample_result.styles) {
          sample_ref.property_overrides.insert_or_assign(key, value);
          sample_ref.property_resets.erase(
              std::remove(sample_ref.property_resets.begin(),
                          sample_ref.property_resets.end(), key),
              sample_ref.property_resets.end());
        }
      }
      const bool has_custom_property_keyframes =
          animation->keyframe_effect()->HasCustomPropertyKeyframes();
      tasm::CustomPropertiesMap custom_property_overrides;
      if (has_custom_property_keyframes) {
        // Custom property declarations such as `--box-padding: 16px` are
        // sampled separately because they are keyed by custom property name,
        // not CSSPropertyID. Normal properties that reference them, for example
        // `padding: var(--box-padding)`, are rebuilt by style resolution after
        // these overrides are applied.
        animation->keyframe_effect()->SampleCustomPropertyKeyframes(
            frame_time, GetKeyframesCustomPropertyMap(animation->name()),
            custom_property_overrides);
        if (!custom_property_overrides.empty()) {
          auto& sample_ref = ensure_sample();
          for (const auto& [key, value] : custom_property_overrides) {
            sample_ref.custom_property_overrides.insert_or_assign(key, value);
            sample_ref.custom_property_resets.erase(
                std::remove(sample_ref.custom_property_resets.begin(),
                            sample_ref.custom_property_resets.end(), key),
                sample_ref.custom_property_resets.end());
          }
          sample_ref.requires_base_style_rebuild = true;
        }
      }
      // Event records come from SampleAt because animation events are tied to
      // timeline progress, not to whether this frame produced normal property
      // styles or custom property overrides.
      if (sample_result.should_send_start_event ||
          sample_result.should_send_end_event ||
          sample_result.iteration_events_due > 0) {
        AnimationSampleForNewPipeline::EventRecord event_record;
        event_record.animation = animation;
        event_record.send_start_event = sample_result.should_send_start_event;
        event_record.send_end_event = sample_result.should_send_end_event;
        event_record.iteration_events_due = sample_result.iteration_events_due;
        pending_event_records_.push_back(std::move(event_record));
      }
      if (sample_result.should_persist_fill_styles &&
          !sample_result.styles.empty()) {
        if (element_ != nullptr) {
          element_->PersistAnimationFillStyles(sample_result.styles);
        }
        for (const auto& [key, value] : sample_result.styles) {
          persisted_property_fill_styles_.insert_or_assign(key, value);
        }
      }
      if (sample_result.should_persist_fill_styles &&
          !custom_property_overrides.empty()) {
        for (const auto& [key, value] : custom_property_overrides) {
          persisted_custom_property_fill_styles_.insert_or_assign(key, value);
        }
      }
      if (sample_result.should_clear_fill_styles) {
        for (const auto& key : animation->GetRawStyleSet()) {
          persisted_property_fill_styles_.erase(key);
          if (element_ != nullptr) {
            element_->ClearPersistedAnimationFillStyle(key);
          }
          auto& sample_ref = ensure_sample();
          sample_ref.property_overrides.erase(key);
          if (std::find(sample_ref.property_resets.begin(),
                        sample_ref.property_resets.end(),
                        key) == sample_ref.property_resets.end()) {
            sample_ref.property_resets.push_back(key);
          }
          sample_ref.requires_base_style_rebuild = true;
        }
        if (has_custom_property_keyframes) {
          for (const auto& key : animation->GetRawCustomPropertySet()) {
            persisted_custom_property_fill_styles_.erase(key);
            auto& sample_ref = ensure_sample();
            sample_ref.custom_property_overrides.erase(key);
            if (std::find(sample_ref.custom_property_resets.begin(),
                          sample_ref.custom_property_resets.end(),
                          key) == sample_ref.custom_property_resets.end()) {
              sample_ref.custom_property_resets.push_back(key);
            }
            sample_ref.requires_base_style_rebuild = true;
          }
        }
      }
    }
  }

  return sample.has_value() ? std::move(*sample)
                            : AnimationSampleForNewPipeline{};
}

AnimationEventRecordsForNewPipeline
CSSKeyframeManager::TakePendingAnimationEventsForNewPipeline() {
  auto pending_event_records = std::move(pending_event_records_);
  pending_event_records_.clear();
  return pending_event_records;
}

void CSSKeyframeManager::QueueCancelEvent(
    const std::shared_ptr<Animation>& animation) {
  if (animation == nullptr) {
    return;
  }
  if (animation->GetState() != Animation::State::kPlay &&
      animation->GetState() != Animation::State::kPause) {
    return;
  }
  AnimationSampleForNewPipeline::EventRecord event_record;
  event_record.animation = animation;
  event_record.send_cancel_event = true;
  pending_event_records_.push_back(std::move(event_record));
}

void CSSKeyframeManager::ClearAnimationEffects(
    const std::shared_ptr<Animation>& animation,
    const tasm::StyleMap* new_base_resolved_styles,
    const tasm::StyleMap* new_underlying_layout_only_styles) {
  if (animation == nullptr) {
    return;
  }

  ClearPersistedFillStyle(animation);

  if (element_ == nullptr) {
    return;
  }

  for (const auto& key : animation->GetRawStyleSet()) {
    element_->ClearPersistedAnimationFillStyle(key);
    std::optional<tasm::CSSValue> value_opt;
    if (tasm::LayoutProperty::IsLayoutOnly(key) &&
        new_underlying_layout_only_styles != nullptr) {
      auto iter = new_underlying_layout_only_styles->find(key);
      if (iter != new_underlying_layout_only_styles->end()) {
        value_opt = iter->second;
      }
    }
    if (!value_opt && new_base_resolved_styles != nullptr) {
      auto iter = new_base_resolved_styles->find(key);
      if (iter != new_base_resolved_styles->end()) {
        value_opt = iter->second;
      }
    }
    if (!value_opt && new_base_resolved_styles == nullptr &&
        new_underlying_layout_only_styles == nullptr) {
      value_opt = element_->GetElementStyle(key);
    }
    if (value_opt) {
      pending_property_overrides_.insert_or_assign(key, std::move(*value_opt));
      pending_property_resets_.erase(
          std::remove(pending_property_resets_.begin(),
                      pending_property_resets_.end(), key),
          pending_property_resets_.end());
    } else {
      pending_property_overrides_.erase(key);
      if (std::find(pending_property_resets_.begin(),
                    pending_property_resets_.end(),
                    key) == pending_property_resets_.end()) {
        pending_property_resets_.push_back(key);
      }
    }
  }

  for (const auto& key : animation->GetRawCustomPropertySet()) {
    if (std::find(pending_custom_property_resets_.begin(),
                  pending_custom_property_resets_.end(),
                  key) == pending_custom_property_resets_.end()) {
      pending_custom_property_resets_.push_back(key);
    }
  }
}

void CSSKeyframeManager::PrepareAnimationRemoval(
    const std::shared_ptr<Animation>& animation,
    const tasm::StyleMap* new_base_resolved_styles,
    const tasm::StyleMap* new_underlying_layout_only_styles) {
  if (animation != nullptr) {
    // This path does not call Destroy(). Retire the Inspector record even for
    // finished animations, which do not dispatch a page cancel event.
    animation->NotifyInspectorCanceled();
  }
  ClearAnimationEffects(animation, new_base_resolved_styles,
                        new_underlying_layout_only_styles);
  QueueCancelEvent(animation);
}

void CSSKeyframeManager::ClearPersistedFillStyle(
    const std::shared_ptr<Animation>& animation) {
  if (animation == nullptr) {
    return;
  }

  for (const auto& key : animation->GetRawStyleSet()) {
    persisted_property_fill_styles_.erase(key);
  }
  for (const auto& key : animation->GetRawCustomPropertySet()) {
    persisted_custom_property_fill_styles_.erase(key);
  }
}

bool CSSKeyframeManager::NeedsFutureTickForNewPipeline() const {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              KEYFRAME_MANAGER_NEEDS_FUTURE_TICK_FOR_NEW_PIPELINE);
  auto has_running_animation = [](const auto& animation_map) {
    return std::any_of(
        animation_map.begin(), animation_map.end(), [](const auto& item) {
          return item.second != nullptr &&
                 item.second->GetState() == Animation::State::kPlay;
        });
  };
  return has_running_animation(animations_map_) ||
         has_running_animation(temp_active_animations_map_) ||
         has_running_animation(temp_keep_animations_map_);
}

Animation::Origin CSSKeyframeManager::ResolveAnimationOrigin(
    const starlight::AnimationData& data) const {
  return element_->HasImperativeAnimationMetadata(data.name)
             ? Animation::Origin::kWebAnimation
             : Animation::Origin::kCSSAnimation;
}

tasm::CSSValue CSSKeyframeManager::ResolveUnderlyingValue(
    tasm::CSSPropertyID id) const {
  auto value = GetStyleInElement(id, element_);
  if (value.IsEmpty()) {
    value = GetDefaultValue(GetPropertyIDToAnimationPropertyTypeMap().at(id));
  }
  return value;
}

std::shared_ptr<Animation> CSSKeyframeManager::CreateAnimation(
    starlight::AnimationData& data,
    const tasm::CustomPropertiesMap* base_custom_properties,
    std::optional<Animation::Origin> origin, bool notify_created) {
  const auto resolved_origin =
      origin.has_value() ? *origin : ResolveAnimationOrigin(data);
  auto parsed_properties = ParseKeyframes(data.name, base_custom_properties);
  const auto animation_timing = ToGfxTimingFunctionData(data.timing_func);
  for (auto& [type, property] : parsed_properties) {
    property.underlying_value = ResolveUnderlyingValue(property.css_id);
    for (auto& parsed : property.keyframes) {
      if (parsed.keyframe->timing_source() ==
          gfx::Keyframe::TimingSource::kAnimation) {
        parsed.keyframe->SetTimingFunction(
            gfx::CreateTimingFunction(animation_timing));
      }
    }
  }
  const auto& custom_property_keyframes =
      GetKeyframesCustomPropertyMap(data.name);
  auto animation = std::make_shared<Animation>(data.name);
  animation->set_animation_data(data);
  animation->SetOrigin(resolved_origin);

  std::unique_ptr<KeyframeEffect> keyframe_effect = KeyframeEffect::Create();
  keyframe_effect->BindAnimationDelegate(this);
  keyframe_effect->BindElement(this->element());
  const bool has_custom_property_keyframes =
      starlight::CSSStyleUtils::HasNonEmptyCSSKeyframesCustomPropertyContent(
          custom_property_keyframes);
  keyframe_effect->SetHasCustomPropertyKeyframes(has_custom_property_keyframes);
  animation->SetKeyframeEffect(std::move(keyframe_effect));
  animation->BindDelegate(this);
  animation->BindElement(this->element());
  SyncAnimationRawCustomPropertySet(animation.get(), custom_property_keyframes);
  for (auto& entry : parsed_properties) {
    animation->SetRawCssId(entry.second.css_id);
    AddParsedPropertyToNewAnimator(std::move(entry.second), animation.get());
  }
  if (HasNoSampleableKeyframes(animation, has_custom_property_keyframes)) {
    LOGE(
        "[animation] skip creating invalid animation without sampleable "
        "keyframes, name:"
        << data.name.str());
    return nullptr;
  }
  if (notify_created) {
    animation->NotifyInspectorCreated();
  }
  return animation;
}

const tasm::CSSKeyframesContent& CSSKeyframeManager::GetKeyframesStyleMap(
    const base::String& animation_name) {
  DCHECK(element() != nullptr);
  const auto& keyframes_map = element()->keyframes_map();
  if (keyframes_map.has_value()) {
    auto iter = keyframes_map->find(animation_name);
    if (iter != keyframes_map->end()) {
      return iter->second->GetKeyframesContent();
    }
  }
  tasm::CSSKeyframesToken* tokens =
      element()->GetCSSKeyframesToken(animation_name);
  if (tokens) {
    return tokens->GetKeyframesContent();
  }
  return GetEmptyKeyframeMap();
}

const tasm::CSSKeyframesCustomPropertyContent&
CSSKeyframeManager::GetKeyframesCustomPropertyMap(
    const base::String& animation_name) {
  DCHECK(element() != nullptr);
  const auto& keyframes_map = element()->keyframes_map();
  if (keyframes_map.has_value()) {
    auto iter = keyframes_map->find(animation_name);
    if (iter != keyframes_map->end()) {
      return iter->second->GetKeyframesCustomPropertyContent();
    }
  }
  tasm::CSSKeyframesToken* tokens =
      element()->GetCSSKeyframesToken(animation_name);
  if (tokens) {
    return tokens->GetKeyframesCustomPropertyContent();
  }
  return GetEmptyCustomPropertyKeyframeMap();
}

CSSKeyframeManager::ParsedPropertyMap CSSKeyframeManager::ParseKeyframes(
    const base::String& animation_name,
    const tasm::CustomPropertiesMap* base_custom_properties) {
  const auto& keyframes_map = GetKeyframesStyleMap(animation_name);
  const auto& keyframe_custom_properties =
      GetKeyframesCustomPropertyMap(animation_name);
  const auto& configs = element_->element_manager()->GetCSSParserConfigs();
  const auto& animation_property_types =
      GetPropertyIDToAnimationPropertyTypeMap();
  const auto* effective_base_custom_properties = base_custom_properties;
  if (effective_base_custom_properties == nullptr &&
      element_->computed_css_style() != nullptr) {
    effective_base_custom_properties =
        element_->computed_css_style()->GetCustomProperties();
  }
  ParsedPropertyMap parsed_properties;
  for (const auto& keyframe_info : keyframes_map) {
    double offset = keyframe_info.first;
    tasm::StyleMap* style_map = keyframe_info.second.get();
    if (!style_map) {
      continue;
    }
    starlight::TimingFunctionData timing_function_for_keyframe;
    const auto timing_iter =
        style_map->find(tasm::kPropertyIDAnimationTimingFunction);
    const bool has_explicit_timing = timing_iter != style_map->end();
    if (has_explicit_timing) {
      auto timing_function_value = timing_iter->second.GetArray()->get(0);
      starlight::CSSStyleUtils::ComputeTimingFunction(
          timing_function_value, false, timing_function_for_keyframe, configs);
    }
    const auto explicit_timing =
        ToGfxTimingFunctionData(timing_function_for_keyframe);
    const auto custom_property_iter =
        keyframe_custom_properties.find(keyframe_info.first);
    const auto* custom_properties =
        custom_property_iter != keyframe_custom_properties.end() &&
                custom_property_iter->second != nullptr
            ? custom_property_iter->second.get()
            : nullptr;
    for (const auto& css_value_pair : *style_map) {
      if (css_value_pair.first == tasm::kPropertyIDAnimationTimingFunction) {
        continue;
      }
      AnimationCurve::CurveType curve_type =
          static_cast<AnimationCurve::CurveType>(css_value_pair.first);
      const auto animation_property =
          animation_property_types.find(css_value_pair.first);
      if (animation_property == animation_property_types.end()) {
        LOGE("[animation] unsupported animation curve type for css:"
             << css_value_pair.first);
        continue;
      }
      auto resolved_value =
          starlight::CSSStyleUtils::ResolveCSSKeyframeValueWithCustomProperties(
              css_value_pair.first, css_value_pair.second, custom_properties,
              configs, effective_base_custom_properties);
      // Resolve animation defaults later; do not allocate a temporary linear
      // function that the animation builder would immediately replace.
      auto timing_function = has_explicit_timing
                                 ? gfx::CreateTimingFunction(explicit_timing)
                                 : nullptr;
      auto parsed =
          CreateParsedKeyframe(curve_type, offset, std::move(timing_function),
                               css_value_pair.first, &resolved_value);
      if (!parsed.keyframe) {
        continue;
      }
      // Explicit linear must stay distinct from an animation-sourced linear,
      // since only the latter follows animation-level timing updates.
      parsed.keyframe->SetTimingSource(
          has_explicit_timing ? gfx::Keyframe::TimingSource::kKeyframe
                              : gfx::Keyframe::TimingSource::kAnimation);
      auto& property = parsed_properties[curve_type];
      property.curve_type = curve_type;
      property.css_id = css_value_pair.first;
      property.gfx_property =
          static_cast<gfx::AnimationPropertyType>(animation_property->second);
      property.has_dynamic_dependencies |= css_value_pair.second.IsVariable();
      if (parsed.keyframe->ValueType() == gfx::KeyframeValueType::kTransform) {
        property.has_dynamic_dependencies |=
            static_cast<const TransformKeyframe*>(parsed.keyframe.get())
                ->HasDynamicDependencies();
      }
      // Insert after frames with the same normalized time to preserve their
      // input order, even when distinct CSS offsets resolve to the same time.
      auto position = property.keyframes.end();
      while (position != property.keyframes.begin() &&
             parsed.keyframe->Time() < (position - 1)->keyframe->Time()) {
        --position;
      }
      property.keyframes.insert(position, std::move(parsed));
    }
  }

  for (auto& [curve_type, property] : parsed_properties) {
    // Entries are added only after successfully parsing a property keyframe,
    // so each property is nonempty and its frame type supports empty endpoints.
    if (property.keyframes.front().keyframe->Offset() != 0.0) {
      property.keyframes.insert(property.keyframes.begin(),
                                CreateParsedKeyframe(curve_type, 0.0, nullptr,
                                                     property.css_id, nullptr));
    }
    if (property.keyframes.back().keyframe->Offset() != 1.0) {
      property.keyframes.push_back(CreateParsedKeyframe(
          curve_type, 1.0, nullptr, property.css_id, nullptr));
    }
  }

  return parsed_properties;
}

void CSSKeyframeManager::RequestNextFrame(std::weak_ptr<Animation> ptr) {
  if (auto animation = ptr.lock()) {
    animation->has_pending_frame_request_ = true;
    element_->RequestNextFrame();
  }
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
          (value.IsArray() && value.GetArray()->size() == 0)) {
        value = GetDefaultValue(starlight::AnimationPropertyType::kTransform);
      }
      break;
    }
    case tasm::kPropertyIDOpacity: {
      if (value.IsNumber() && value.GetNumber() < 0.0f) {
        return;
      }
      break;
    }
    case tasm::kPropertyIDVisibility: {
      if (!value.IsEnum()) {
        break;
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

void CSSKeyframeManager::SetNeedsAnimationStyleRecalc(
    const base::String& name) {
  // clear effect
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_MANAGER_NEEDS_ANIMATION_RECALC);
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
        if (!animation->GetTransitionFlag()) {
          element_->ClearPersistedAnimationFillStyle(key);
        }
        std::optional<tasm::CSSValue> value_opt =
            element_->GetElementStyle(key);
        if (!value_opt) {
          reset_origin_css_styles[key] = tasm::CSSValue();
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

const tasm::CSSKeyframesContent& CSSKeyframeManager::GetEmptyKeyframeMap() {
  static base::NoDestructor<tasm::CSSKeyframesContent> kEmptyKeyframeMap;
  return *kEmptyKeyframeMap.get();
}

const tasm::CSSKeyframesCustomPropertyContent&
CSSKeyframeManager::GetEmptyCustomPropertyKeyframeMap() {
  static base::NoDestructor<tasm::CSSKeyframesCustomPropertyContent>
      kEmptyCustomPropertyKeyframeMap;
  return *kEmptyCustomPropertyKeyframeMap.get();
}

tasm::CSSValue CSSKeyframeManager::GetDefaultValue(
    starlight::AnimationPropertyType type) {
  if (GetLayoutPropertyTypeSet().count(type) != 0) {
    // the default values of layout properties are 'auto'.
    return tasm::CSSValue();
  } else if (type == starlight::AnimationPropertyType::kOpacity) {
    return tasm::CSSValue(kDefaultOpacity, tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kBackgroundColor ||
             (type >= starlight::AnimationPropertyType::kBorderTopColor &&
              type <= starlight::AnimationPropertyType::kBorderBottomColor)) {
    return tasm::CSSValue(kDefaultBackgroundColor,
                          tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kColor) {
    return tasm::CSSValue(kDefaultTextColor, tasm::CSSValuePattern::NUMBER);
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
    return tasm::CSSValue(kDefaultFloatValue, tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kBoxShadow) {
    return tasm::CSSValue(lepus::CArray::Create());
  }
  return tasm::CSSValue();
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
