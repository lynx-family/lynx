// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_CSS_KEYFRAME_MANAGER_H_
#define CORE_ANIMATION_CSS_KEYFRAME_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "core/animation/animation.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/animation_data.h"

namespace lynx {

namespace shell {
class VSyncMonitor;
}

namespace tasm {
class Element;
class CSSKeyframesToken;
}  // namespace tasm
namespace animation {

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

class CSSKeyframeManager : public AnimationDelegate {
 public:
  static const tasm::CssMeasureContext& GetLengthContext(
      tasm::Element* element);

  CSSKeyframeManager(tasm::Element* element);
  ~CSSKeyframeManager() = default;

  void SetAnimationDataAndPlay(
      std::vector<starlight::AnimationData>& anim_data);

  virtual void TickAllAnimation(fml::TimePoint& time);

  void RequestNextFrame(std::weak_ptr<Animation> ptr) override;

  void UpdateFinalStyleMap(const tasm::StyleMap& styles) override;

  void FlushAnimatedStyle() override;

  void NotifyClientAnimated(tasm::StyleMap& styles, tasm::CSSValue value,
                            tasm::CSSPropertyID css_id) override;
  void SetNeedsAnimationStyleRecalc(const std::string& name) override;

  bool InitCurveAndModelAndKeyframe(
      AnimationCurve::CurveType type, Animation* animation, double offset,
      std::unique_ptr<TimingFunction> timing_function,
      std::pair<tasm::CSSPropertyID, tasm::CSSValue> css_value_pair);

  KeyframeModel* ConstructModel(std::unique_ptr<AnimationCurve> curve,
                                AnimationCurve::CurveType type,
                                Animation* animation);
  bool SetKeyframeValue(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& css_value_pair);

  virtual tasm::CSSKeyframesContent& GetKeyframesStyleMap(
      const std::string& animation_name);

  tasm::CSSKeyframesContent& empty_keyframe_map() {
    return empty_keyframe_map_;
  }

  static tasm::CSSValue GetDefaultValue(starlight::AnimationPropertyType type);

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

 protected:
  std::shared_ptr<Animation> CreateAnimation(starlight::AnimationData& data);

  std::vector<starlight::AnimationData> animation_data_;
  // animations of css style
  std::unordered_map<base::String, std::shared_ptr<Animation>> animations_map_;
  // save active animations for play and clear
  std::unordered_map<base::String, std::shared_ptr<Animation>>
      temp_active_animations_map_;

 private:
  void MakeKeyframeModel(Animation* animation,
                         const std::string& animation_name);

 private:
  std::shared_ptr<shell::VSyncMonitor> vsync_monitor_{nullptr};
  tasm::CSSKeyframesContent empty_keyframe_map_;
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_CSS_KEYFRAME_MANAGER_H_
