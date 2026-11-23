// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_CSS_TRANSITION_MANAGER_H_
#define CORE_ANIMATION_CSS_TRANSITION_MANAGER_H_

#include <memory>
#include <string>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/style/transition_data.h"

namespace lynx {
namespace tasm {
class CssMeasureContext;
}  // namespace tasm

namespace starlight {
struct CanonicalComputedValue;
}  // namespace starlight
namespace animation {

const char* ConvertAnimationPropertyTypeToString(
    lynx::starlight::AnimationPropertyType type);

base::flex_optional<tasm::CSSValue> ConvertCanonicalComputedValueForTransition(
    tasm::CSSPropertyID css_id, const starlight::CanonicalComputedValue& value,
    const tasm::CssMeasureContext& context);

class CSSTransitionManager : public CSSKeyframeManager {
 public:
  CSSTransitionManager(tasm::Element* element);
  ~CSSTransitionManager() override;

  void setTransitionData(const starlight::TransitionData& transition_data);

  const tasm::CSSKeyframesContent& GetKeyframesStyleMap(
      const base::String& animation_name) override;

  void TickAllAnimation(fml::TimePoint& time) override;

  bool ConsumeCSSProperty(tasm::CSSPropertyID css_id,
                          const tasm::CSSValue& end_value);

  void UpdateTransitionsForNewPipeline(
      const starlight::ComputedCSSStyle& previous_base_style,
      const starlight::ComputedCSSStyle& previous_final_style,
      const starlight::ComputedCSSStyle& new_base_style,
      const tasm::StyleMap& previous_underlying_layout_only_styles,
      const tasm::StyleMap& new_underlying_layout_only_styles);
  AnimationSampleForNewPipeline CollectTransitionUpdatesForNewPipeline(
      fml::TimePoint& time);
  bool NeedsFutureTickForNewPipeline() const;

  bool NeedsTransition(tasm::CSSPropertyID css_id);

  bool HasActiveTransition(tasm::CSSPropertyID css_id);

  bool ConsumeCSSPropertyForActiveTransition(tasm::CSSPropertyID css_id,
                                             const tasm::CSSValue& end_value);

  void ClearPreviousEndValue(tasm::CSSPropertyID css_id);

 private:
  struct PlatformTransition {
    tasm::CSSPropertyID property;
    PlatformAnimationPtr state{};
    fml::RefPtr<gfx::PlatformTransitionSnapshot> pending_snapshot{};
  };

  PlatformTransition* FindPlatformTransition(tasm::CSSPropertyID css_id);
  void RemovePlatformTransition(PlatformTransition& transition,
                                bool clear_previous_end_value);
  void ApplyReadyPlatformFallbacks();
  void StartCoreTransition(std::shared_ptr<Animation>&& animation,
                           bool play_handles_initial_frame);
  void StopTransitionAnimator(tasm::CSSPropertyID css_id,
                              starlight::AnimationPropertyType property_type,
                              bool play_handles_initial_frame);
  void CancelPlatformTransition(tasm::CSSPropertyID css_id);
  void PrepareTransitionRemovalCleanup(
      const std::shared_ptr<Animation>& animation);
  void SyncTransitionData(const starlight::TransitionData& transition_data,
                          bool use_legacy_destroy_path);
  bool UpdateTransitionAnimator(tasm::CSSPropertyID css_id,
                                starlight::AnimationPropertyType property_type,
                                tasm::CSSValue&& start_value,
                                tasm::CSSValue&& end_value,
                                bool play_handles_initial_frame,
                                bool allow_platform = false);
  bool IsValueValid(starlight::AnimationPropertyType type,
                    const tasm::CSSValue& value,
                    const tasm::CSSParserConfigs& configs);
  void SetTransitionDataInternal(
      starlight::AnimationPropertyType property, long duration, long delay,
      const starlight::TimingFunctionData& timing_func,
      base::LinearFlatMap<base::String, std::shared_ptr<Animation>>&
          active_animations_map);

  static starlight::AnimationPropertyType GetAnimationPropertyType(
      tasm::CSSPropertyID id);

  bool IsShouldTransitionType(starlight::AnimationPropertyType type);

  PlatformTransition platform_transitions_[2]{{tasm::kPropertyIDOpacity},
                                              {tasm::kPropertyIDTransform}};

 protected:
  Animation::Origin ResolveAnimationOrigin(
      const starlight::AnimationData&) const override {
    return Animation::Origin::kCSSTransition;
  }

  gfx::AnimationKind GetAnimationKind() const override {
    return gfx::AnimationKind::kTransition;
  }
  base::LinearFlatMap<unsigned int, starlight::AnimationData> transition_data_;
  base::LinearFlatMap<base::String, tasm::CSSKeyframesContent> keyframe_tokens_;
  base::LinearFlatSet<unsigned int> property_types_;
  tasm::StyleMap previous_end_values_;
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_CSS_TRANSITION_MANAGER_H_
