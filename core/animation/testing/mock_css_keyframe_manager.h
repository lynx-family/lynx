// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TESTING_MOCK_CSS_KEYFRAME_MANAGER_H_
#define CORE_ANIMATION_TESTING_MOCK_CSS_KEYFRAME_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "core/animation/animation.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/style/animation_data.h"

namespace lynx {

namespace animation {

class MockCSSKeyframeManager : public CSSKeyframeManager {
 public:
  MockCSSKeyframeManager(tasm::Element* element)
      : CSSKeyframeManager(element) {}
  ~MockCSSKeyframeManager() = default;
  std::unordered_map<base::String, std::shared_ptr<Animation>>&
  animations_map() {
    return animations_map_;
  }

  void SetNeedsAnimationStyleRecalc(const std::string& name) override {
    clear_effect_animation_name_ = name;
  }

  const std::string& GetClearEffectAnimationName() {
    return clear_effect_animation_name_;
  }

  void RequestNextFrame(std::weak_ptr<Animation> ptr) override {
    has_request_next_frame_ = true;
  }

  void FlushAnimatedStyle() override { has_flush_animated_style_ = true; }

  bool has_flush_animated_style() { return has_flush_animated_style_; }

  bool has_request_next_frame() { return has_request_next_frame_; }

  void ClearUTStatus();

 private:
  std::string clear_effect_animation_name_;
  bool has_flush_animated_style_{false};
  bool has_request_next_frame_{false};
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_TESTING_MOCK_CSS_KEYFRAME_MANAGER_H_
