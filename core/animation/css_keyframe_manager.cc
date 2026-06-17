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
#include "core/animation/animation_delegate.h"
#include "core/animation/animation_trace_event_def.h"
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

KeyframeModel* CSSKeyframeManager::ConstructModel(
    std::unique_ptr<AnimationCurve> curve, AnimationCurve::CurveType type,
    Animation* animation) {
  curve->SetElement(element_);
  // add type here
  curve->type_ = type;
  std::unique_ptr<KeyframeModel> new_keyframe_model =
      KeyframeModel::Create(std::move(curve));
  new_keyframe_model->UpdateAnimationData(&animation->get_animation_data());
  KeyframeModel* keyframe_model = new_keyframe_model.get();
  animation->keyframe_effect()->AddKeyframeModel(std::move(new_keyframe_model));
  return keyframe_model;
}

bool CSSKeyframeManager::InitCurveAndModelAndKeyframe(
    AnimationCurve::CurveType type, Animation* animation, double offset,
    std::unique_ptr<gfx::TimingFunction> timing_function,
    tasm::CSSPropertyID id, const tasm::CSSValue& value) {
  KeyframeModel* keyframe_model =
      animation->keyframe_effect()->GetKeyframeModelByCurveType(type);
  bool has_model = (keyframe_model != nullptr);
  std::unique_ptr<AnimationCurve> new_curve;
  std::unique_ptr<gfx::Keyframe> keyframe;
  KeyframeCallbacks keyframe_callbacks;
  auto init_keyframe = [&](auto factory) -> bool {
    auto typed_keyframe = factory();
    if (!typed_keyframe->SetValue(id, value, element_)) {
      return false;
    }
    keyframe_callbacks = MakeKeyframeCallbacks(typed_keyframe.get());
    keyframe = std::move(typed_keyframe);
    return true;
  };
  if (GetLayoutCurveTypeSet().count(type) != 0) {
    if (!has_model) {
      new_curve = KeyframedLayoutAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return LayoutKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                        std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::OPACITY) {
    if (!has_model) {
      new_curve = KeyframedOpacityAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return OpacityKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                         std::move(timing_function));
        })) {
      return false;
    }
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
    if (!init_keyframe([&]() {
          return ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                       std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::FLEX_GROW ||
             type == AnimationCurve::CurveType::OFFSET_DISTANCE) {
    if (!has_model) {
      new_curve = KeyframedFloatAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                       std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::FILTER) {
    if (!has_model) {
      new_curve = KeyframedFilterAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return FilterKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                        std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::TRANSFORM) {
    if (!has_model) {
      new_curve = KeyframedTransformAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return TransformKeyframe::Create(fml::TimeDelta::FromSecondsF(offset),
                                           std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::BACKGROUND_POSITION) {
    if (!has_model) {
      new_curve = KeyframedBackgroundPositionAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return BackgroundPositionKeyframe::Create(
              fml::TimeDelta::FromSecondsF(offset), std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::TRANSFORM_ORIGIN) {
    if (!has_model) {
      new_curve = KeyframedTransformOriginAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return TransformOriginKeyframe::Create(
              fml::TimeDelta::FromSecondsF(offset), std::move(timing_function));
        })) {
      return false;
    }
  } else if (type == AnimationCurve::CurveType::VISIBILITY) {
    if (!has_model) {
      new_curve = KeyframedVisibilityAnimationCurve::Create();
    }
    if (!init_keyframe([&]() {
          return VisibilityKeyframe::Create(
              fml::TimeDelta::FromSecondsF(offset), std::move(timing_function));
        })) {
      return false;
    }
  } else {
    return false;
  }
  // construct keyframe_model with AnimationCurve
  if (!has_model) {
    keyframe_model = ConstructModel(std::move(new_curve), type, animation);
  }
  // add keyframe into AnimationCurve
  keyframe_model->animation_curve()->AddKeyframe(std::move(keyframe),
                                                 keyframe_callbacks);
  return true;
}

void CSSKeyframeManager::TickAllAnimation(fml::TimePoint& frame_time) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, KEYFRAME_MANAGER_TICK_ALL_ANIMATION);
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
    base::Vector<starlight::AnimationData>& anim_data) {
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
        if (use_new_pipeline_cleanup) {
          PrepareAnimationRemoval(animation->second, new_base_resolved_styles,
                                  new_underlying_layout_only_styles);
        } else {
          animation->second->Destroy();
        }
        auto recreated_animation =
            CreateAnimation(data, new_base_custom_properties);
        if (recreated_animation != nullptr) {
          temp_active_animations_map_[data.name] = recreated_animation;
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
        if (use_new_pipeline_cleanup) {
          PrepareAnimationRemoval(animation->second, new_base_resolved_styles,
                                  new_underlying_layout_only_styles);
        } else {
          animation->second->Destroy();
        }
        auto recreated_animation =
            CreateAnimation(data, new_base_custom_properties);
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
      active_ani_iter.second->Pause();
    } else {
      active_ani_iter.second->Play(play_handles_initial_frame);
    }
  }
  // 3. Swap active animations to animations_map_.
  animations_map_.swap(temp_active_animations_map_);
  animations_map_.merge(temp_keep_animations_map_);
  temp_keep_animations_map_.clear();
  temp_active_animations_map_.clear();
}

void CSSKeyframeManager::SyncAnimationDataForNewPipeline(
    base::Vector<starlight::AnimationData>& anim_data, bool force_rebuild,
    const tasm::StyleMap* new_base_resolved_styles,
    const tasm::StyleMap* new_underlying_layout_only_styles,
    const tasm::CustomPropertiesMap* new_base_custom_properties) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              KEYFRAME_MANAGER_SYNC_ANIMATION_DATA_FOR_NEW_PIPELINE);
  SetAnimationDataAndPlayInternal(
      anim_data, force_rebuild, false, true, new_base_resolved_styles,
      new_underlying_layout_only_styles, new_base_custom_properties);
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
    for (auto& item : animations_map_) {
      auto& animation = item.second;
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
            frame_time, GetKeyframesCustomPropertyMap(item.first),
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

std::shared_ptr<Animation> CSSKeyframeManager::CreateAnimation(
    starlight::AnimationData& data,
    const tasm::CustomPropertiesMap* base_custom_properties) {
  // 1. create animation & keyframe_effect according to animation data
  auto animation = std::make_shared<Animation>(data.name);
  animation->set_animation_data(data);

  std::unique_ptr<KeyframeEffect> keyframe_effect = KeyframeEffect::Create();
  keyframe_effect->BindAnimationDelegate(this);
  keyframe_effect->BindElement(this->element());
  const bool has_custom_property_keyframes =
      starlight::CSSStyleUtils::HasNonEmptyCSSKeyframesCustomPropertyContent(
          GetKeyframesCustomPropertyMap(data.name));
  keyframe_effect->SetHasCustomPropertyKeyframes(has_custom_property_keyframes);
  animation->SetKeyframeEffect(std::move(keyframe_effect));
  animation->BindDelegate(this);
  animation->BindElement(this->element());
  // 2. create keyframe Models& animation Curves according to CSS keyframe
  // tokens
  MakeKeyframeModel(animation.get(), data.name, base_custom_properties);
  if (HasNoSampleableKeyframes(animation, has_custom_property_keyframes)) {
    LOGE(
        "[animation] skip creating invalid animation without sampleable "
        "keyframes, name:"
        << data.name.str());
    return nullptr;
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

void CSSKeyframeManager::MakeKeyframeModel(
    Animation* animation, const base::String& animation_name,
    const tasm::CustomPropertiesMap* base_custom_properties) {
  const auto& keyframes_map = GetKeyframesStyleMap(animation_name);
  const auto& keyframe_custom_properties =
      GetKeyframesCustomPropertyMap(animation_name);
  SyncAnimationRawCustomPropertySet(animation, keyframe_custom_properties);
  const auto& configs = element_->element_manager()->GetCSSParserConfigs();
  const auto* effective_base_custom_properties = base_custom_properties;
  if (effective_base_custom_properties == nullptr &&
      element_->computed_css_style() != nullptr) {
    effective_base_custom_properties =
        element_->computed_css_style()->GetCustomProperties();
  }
  for (const auto& keyframe_info : keyframes_map) {
    double offset = keyframe_info.first;
    tasm::StyleMap* style_map = keyframe_info.second.get();
    if (!style_map) {
      continue;
    }
    std::unique_ptr<gfx::TimingFunction> timing_function = nullptr;
    starlight::TimingFunctionData timing_function_for_keyframe;
    const auto& iter =
        style_map->find(tasm::kPropertyIDAnimationTimingFunction);
    if (iter != style_map->end()) {
      auto timing_function_value = iter->second.GetArray()->get(0);
      starlight::CSSStyleUtils::ComputeTimingFunction(
          timing_function_value, false, timing_function_for_keyframe,
          element_->element_manager()->GetCSSParserConfigs());
    }
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
      timing_function = gfx::CreateTimingFunction(
          ToGfxTimingFunctionData(timing_function_for_keyframe));
      AnimationCurve::CurveType curve_type =
          static_cast<AnimationCurve::CurveType>(css_value_pair.first);
      if (GetPropertyIDToAnimationPropertyTypeMap().find(
              css_value_pair.first) ==
          GetPropertyIDToAnimationPropertyTypeMap().end()) {
        LOGE("[animation] unsupported animation curve type for css:"
             << css_value_pair.first);
        continue;
      }
      auto resolved_value =
          starlight::CSSStyleUtils::ResolveCSSKeyframeValueWithCustomProperties(
              css_value_pair.first, css_value_pair.second, custom_properties,
              configs, effective_base_custom_properties);
      bool init_status = InitCurveAndModelAndKeyframe(
          curve_type, animation, offset, std::move(timing_function),
          css_value_pair.first, resolved_value);
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
    return tasm::CSSValue(OpacityKeyframe::kDefaultOpacity,
                          tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kBackgroundColor ||
             (type >= starlight::AnimationPropertyType::kBorderTopColor &&
              type <= starlight::AnimationPropertyType::kBorderBottomColor)) {
    return tasm::CSSValue(ColorKeyframe::kDefaultBackgroundColor,
                          tasm::CSSValuePattern::NUMBER);
  } else if (type == starlight::AnimationPropertyType::kColor) {
    return tasm::CSSValue(ColorKeyframe::kDefaultTextColor,
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
    return tasm::CSSValue(FloatKeyframe::kDefaultFloatValue,
                          tasm::CSSValuePattern::NUMBER);
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
