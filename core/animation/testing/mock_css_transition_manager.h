// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TESTING_MOCK_CSS_TRANSITION_MANAGER_H_
#define CORE_ANIMATION_TESTING_MOCK_CSS_TRANSITION_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/animation/css_transition_manager.h"
#include "core/style/transition_data.h"

namespace lynx {
namespace animation {

class MockCSSTransitionManager : public CSSTransitionManager {
 public:
  MockCSSTransitionManager(tasm::Element* element)
      : CSSTransitionManager(element) {}
  ~MockCSSTransitionManager() = default;

  std::unordered_map<unsigned int, starlight::AnimationData>&
  transition_data() {
    return transition_data_;
  }

  std::vector<starlight::AnimationData>& animation_data() {
    return animation_data_;
  }

  std::unordered_set<unsigned int>& property_types() { return property_types_; }

  std::unordered_map<std::string, tasm::CSSKeyframesContent>&
  keyframe_tokens() {
    return keyframe_tokens_;
  }

  std::unordered_map<base::String, std::shared_ptr<Animation>>&
  animations_map() {
    return animations_map_;
  }

  void NotifyClientAnimated(tasm::StyleMap& styles, tasm::CSSValue value,
                            tasm::CSSPropertyID css_id) override {
    has_been_ticked_ = true;
  }

  void SetNeedsAnimationStyleRecalc(const std::string& name) override {
    clear_effect_animation_name_ = name;
  }

  bool has_been_ticked() { return has_been_ticked_; }

  const std::string& GetClearEffectAnimationName() {
    return clear_effect_animation_name_;
  }

 private:
  bool has_been_ticked_{false};
  std::string clear_effect_animation_name_;
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_TESTING_MOCK_CSS_TRANSITION_MANAGER_H_
