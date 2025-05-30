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
namespace animation {

const char* ConvertAnimationPropertyTypeToString(
    lynx::starlight::AnimationPropertyType type);

class CSSTransitionManager : public CSSKeyframeManager {
 public:
  CSSTransitionManager(tasm::Element* element) : CSSKeyframeManager(element) {}
  ~CSSTransitionManager() = default;

  void setTransitionData(
      const base::Vector<starlight::TransitionData>& transition_data);

  tasm::CSSKeyframesContent& GetKeyframesStyleMap(
      const base::String& animation_name) override;

  void TickAllAnimation(fml::TimePoint& time) override;

  bool ConsumeCSSProperty(tasm::CSSPropertyID css_id,
                          const tasm::CSSValue& end_value);

  bool NeedsTransition(tasm::CSSPropertyID css_id);

  void ClearPreviousEndValue(tasm::CSSPropertyID css_id);

 private:
  void TryToStopTransitionAnimator(
      starlight::AnimationPropertyType property_type);
  bool IsValueValid(starlight::AnimationPropertyType type,
                    const tasm::CSSValue& value,
                    const tasm::CSSParserConfigs& configs);
  void SetTransitionDataInternal(
      const starlight::TransitionData& data,
      base::LinearFlatMap<base::String, std::shared_ptr<Animation>>&
          active_animations_map);

  static starlight::AnimationPropertyType GetAnimationPropertyType(
      tasm::CSSPropertyID id);

  bool IsShouldTransitionType(starlight::AnimationPropertyType type);

 protected:
  base::LinearFlatMap<unsigned int, starlight::AnimationData> transition_data_;
  base::LinearFlatMap<base::String, tasm::CSSKeyframesContent> keyframe_tokens_;
  base::LinearFlatSet<unsigned int> property_types_;
  tasm::StyleMap previous_end_values_;
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_CSS_TRANSITION_MANAGER_H_
