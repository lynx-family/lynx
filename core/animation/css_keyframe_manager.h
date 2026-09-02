// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_CSS_KEYFRAME_MANAGER_H_
#define CORE_ANIMATION_CSS_KEYFRAME_MANAGER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "core/animation/animation.h"
#include "core/animation/animation_backend_evaluator.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/animation_data.h"
#include "gfx/animation/platform_animation.h"
#include "gfx/animation/timing_function.h"

namespace lynx {

namespace base {
class VSyncMonitor;
}

namespace tasm {
class Element;
class CSSKeyframesToken;
class CssMeasureContext;
}  // namespace tasm
namespace animation {

base::flex_optional<tasm::CSSValue> ConvertCanonicalComputedValueForAnimation(
    tasm::CSSPropertyID css_id, const starlight::CanonicalComputedValue& value,
    const tasm::CssMeasureContext& context);

const std::unordered_set<starlight::AnimationPropertyType>&
GetLayoutPropertyTypeSet();
const std::unordered_set<AnimationCurve::CurveType>& GetLayoutCurveTypeSet();
const std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>&
GetPropertyIDToAnimationPropertyTypeMap();
const std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>&
GetPolymericPropertyIDToAnimationPropertyTypeMap(
    starlight::AnimationPropertyType polymeric_type);

const std::unordered_set<tasm::CSSPropertyID>& GetAnimatablePropertyIDSet();
// Check that is this property a animatable property for new animator.
bool IsAnimatableProperty(tasm::CSSPropertyID css_id);

struct AnimationSampleForNewPipeline {
  struct EventRecord {
    std::shared_ptr<Animation> animation;
    bool send_start_event{false};
    bool send_end_event{false};
    int iteration_events_due{0};
    bool send_cancel_event{false};
  };

  tasm::StyleMap property_overrides;
  tasm::CustomPropertiesMap custom_property_overrides;
  std::vector<tasm::CSSPropertyID> property_resets;
  std::vector<base::String> custom_property_resets;
  bool requires_base_style_rebuild{false};

  bool empty() const {
    return property_overrides.empty() && custom_property_overrides.empty() &&
           property_resets.empty() && custom_property_resets.empty() &&
           !requires_base_style_rebuild;
  }
};

using AnimationEventRecordsForNewPipeline =
    base::InlineVector<AnimationSampleForNewPipeline::EventRecord, 2>;

class CSSKeyframeManager : public AnimationDelegate {
 public:
  static const tasm::CssMeasureContext& GetLengthContext(
      tasm::Element* element);

  CSSKeyframeManager(tasm::Element* element);
  ~CSSKeyframeManager() = default;

  void AddAnimationDataAndPlay(
      base::Vector<starlight::AnimationData>& anim_data);

  void SetAnimationDataAndPlay(
      base::Vector<starlight::AnimationData>& anim_data);

  void SyncAnimationDataForNewPipeline(
      base::Vector<starlight::AnimationData>& anim_data,
      bool force_rebuild = false,
      const tasm::StyleMap* new_base_resolved_styles = nullptr,
      const tasm::StyleMap* new_underlying_layout_only_styles = nullptr,
      const tasm::CustomPropertiesMap* new_base_custom_properties = nullptr,
      const starlight::ComputedCSSStyle* new_base_style = nullptr);

  void UpdateUnderlyingValue(tasm::CSSPropertyID id,
                             const tasm::CSSValue& value);

  void UpdateUnderlyingValueFromComputedStyle(
      tasm::CSSPropertyID id,
      const starlight::ComputedCSSStyle& computed_style);

  AnimationSampleForNewPipeline CollectAnimationUpdatesForNewPipeline(
      fml::TimePoint& time);

  AnimationEventRecordsForNewPipeline
  TakePendingAnimationEventsForNewPipeline();

  bool NeedsFutureTickForNewPipeline() const;

  virtual void TickAllAnimation(fml::TimePoint& time);

  void RequestNextFrame(std::weak_ptr<Animation> ptr) override;

  void UpdateFinalStyleMap(const tasm::StyleMap& styles) override;

  void FlushAnimatedStyle() override;

  void NotifyClientAnimated(tasm::StyleMap& styles, tasm::CSSValue value,
                            tasm::CSSPropertyID css_id) override;
  void SetNeedsAnimationStyleRecalc(const base::String& name) override;

  bool InitCurveAndModelAndKeyframe(
      AnimationCurve::CurveType type, Animation* animation, double offset,
      std::unique_ptr<gfx::TimingFunction> timing_function,
      tasm::CSSPropertyID id, const tasm::CSSValue& value);

  KeyframeModel* ConstructModel(std::unique_ptr<AnimationCurve> curve,
                                AnimationCurve::CurveType type,
                                Animation* animation);
  bool SetKeyframeValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair);

  virtual const tasm::CSSKeyframesContent& GetKeyframesStyleMap(
      const base::String& animation_name);
  virtual const tasm::CSSKeyframesCustomPropertyContent&
  GetKeyframesCustomPropertyMap(const base::String& animation_name);

  static const tasm::CSSKeyframesContent& GetEmptyKeyframeMap();
  static const tasm::CSSKeyframesCustomPropertyContent&
  GetEmptyCustomPropertyKeyframeMap();

  static tasm::CSSValue GetDefaultValue(starlight::AnimationPropertyType type);

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

 protected:
  void SetAnimationDataAndPlayInternal(
      base::Vector<starlight::AnimationData>& anim_data, bool force_rebuild,
      bool play_handles_initial_frame, bool use_new_pipeline_cleanup,
      const tasm::StyleMap* new_base_resolved_styles = nullptr,
      const tasm::StyleMap* new_underlying_layout_only_styles = nullptr,
      const tasm::CustomPropertiesMap* new_base_custom_properties = nullptr);

  void QueueCancelEvent(const std::shared_ptr<Animation>& animation);

  void ClearAnimationEffects(
      const std::shared_ptr<Animation>& animation,
      const tasm::StyleMap* new_base_resolved_styles,
      const tasm::StyleMap* new_underlying_layout_only_styles);

  void PrepareAnimationRemoval(
      const std::shared_ptr<Animation>& animation,
      const tasm::StyleMap* new_base_resolved_styles,
      const tasm::StyleMap* new_underlying_layout_only_styles);

  void ClearPersistedFillStyle(const std::shared_ptr<Animation>& animation);

  void UpdateUnderlyingValuesFromComputedStyle(
      const starlight::ComputedCSSStyle& computed_style,
      const tasm::StyleMap* underlying_layout_only_styles);

  virtual gfx::AnimationKind GetAnimationKind() const {
    return gfx::AnimationKind::kKeyframe;
  }

  base::InlineVector<starlight::AnimationData, 1> animation_data_;
  // The collection of animations running on the current element.
  base::LinearFlatMap<base::String, std::shared_ptr<Animation>> animations_map_;
  // The collection of animations that no need to update states during the diff.
  base::LinearFlatMap<base::String, std::shared_ptr<Animation>>
      temp_keep_animations_map_;
  // The collection of animations that need to update states during the diff.
  base::LinearFlatMap<base::String, std::shared_ptr<Animation>>
      temp_active_animations_map_;
  tasm::StyleMap pending_property_overrides_;
  std::vector<tasm::CSSPropertyID> pending_property_resets_;
  std::vector<base::String> pending_custom_property_resets_;
  tasm::StyleMap persisted_property_fill_styles_;
  tasm::CustomPropertiesMap persisted_custom_property_fill_styles_;
  AnimationEventRecordsForNewPipeline pending_event_records_;

  struct PlatformAnimationState {
    gfx::AnimationId animation_id{0};
    uint32_t generation{1};
    starlight::AnimationData animation_data;
    std::vector<gfx::PlatformAnimationProperty> properties;
    bool handed_off{false};
  };

  struct AnimationBuildResult {
    std::shared_ptr<Animation> core_animation;
    std::optional<PlatformAnimationState> platform_animation;
  };

  AnimationBuildResult BuildAnimation(
      starlight::AnimationData& data,
      const tasm::CustomPropertiesMap* base_custom_properties = nullptr);
  void QueuePlatformAnimationCommands(PlatformAnimationState& animation,
                                      gfx::PlatformAnimationCommandType type);

 private:
  struct ParsedKeyframe {
    std::unique_ptr<gfx::Keyframe> keyframe;
    KeyframeCallbacks callbacks;
  };

  struct ParsedPropertyKeyframes {
    AnimationCurve::CurveType curve_type{AnimationCurve::CurveType::UNSUPPORT};
    tasm::CSSPropertyID css_id{tasm::kPropertyStart};
    gfx::AnimationPropertyType gfx_property{gfx::AnimationPropertyType::kNone};
    std::vector<ParsedKeyframe> keyframes;
    bool has_dynamic_dependencies{false};
  };

  using ParsedPropertyMap =
      std::map<AnimationCurve::CurveType, ParsedPropertyKeyframes>;

  struct EndpointReplacement {
    ParsedKeyframe* endpoint{nullptr};
    ParsedKeyframe original;
  };

  ParsedKeyframe CreateParsedKeyframe(
      AnimationCurve::CurveType type, double offset,
      std::unique_ptr<gfx::TimingFunction> timing_function,
      tasm::CSSPropertyID id, const tasm::CSSValue* value);
  std::unique_ptr<AnimationCurve> CreateCurve(AnimationCurve::CurveType type);
  void AddParsedPropertyToCore(ParsedPropertyKeyframes property,
                               Animation* animation);
  ParsedPropertyMap ParseKeyframes(
      const base::String& animation_name,
      const tasm::CustomPropertiesMap* base_custom_properties);
  bool MaterializePlatformEndpoints(
      ParsedPropertyMap& properties,
      std::vector<EndpointReplacement>& replacements);
  static void RestoreEmptyEndpoints(
      std::vector<EndpointReplacement>& replacements);
  AnimationBackendResult EvaluatePlatformSupport(
      const ParsedPropertyMap& properties,
      const starlight::AnimationData& animation_data,
      bool has_custom_property_keyframes) const;
  std::shared_ptr<Animation> CreateCoreAnimation(
      starlight::AnimationData& data, ParsedPropertyMap properties,
      const tasm::CSSKeyframesCustomPropertyContent& custom_property_keyframes);
  PlatformAnimationState CreatePlatformAnimationState(
      starlight::AnimationData& data, ParsedPropertyMap properties);
  gfx::PlatformAnimationCommand BuildPlatformAnimationCommand(
      const PlatformAnimationState& animation,
      gfx::PlatformAnimationCommandType type) const;
  std::shared_ptr<base::VSyncMonitor> vsync_monitor_{nullptr};
  std::unordered_map<base::String, PlatformAnimationState> platform_animations_;
  std::unordered_map<base::String, PlatformAnimationState>
      temp_active_platform_animations_;
  std::unordered_map<base::String, PlatformAnimationState>
      temp_keep_platform_animations_;
  gfx::AnimationId next_platform_animation_id_{1};
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_CSS_KEYFRAME_MANAGER_H_
