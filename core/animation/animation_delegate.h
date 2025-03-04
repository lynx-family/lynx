// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_ANIMATION_DELEGATE_H_
#define CORE_ANIMATION_ANIMATION_DELEGATE_H_

#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/linked_hash_map.h"
#include "core/renderer/css/css_property.h"

namespace lynx {
namespace animation {

class Animation;
class AnimationDelegate {
 public:
  virtual ~AnimationDelegate() {}
  virtual void RequestNextFrame(std::weak_ptr<Animation> ptr){};
  virtual void UpdateFinalStyleMap(const tasm::StyleMap& styles){};
  virtual void FlushAnimatedStyle(){};
  virtual void SetNeedsAnimationStyleRecalc(const std::string& name){};
  virtual void NotifyClientAnimated(tasm::StyleMap& styles,
                                    tasm::CSSValue value,
                                    tasm::CSSPropertyID css_id){};
  tasm::Element* element() { return element_; }

 protected:
  std::vector<std::weak_ptr<Animation>> active_animations_;
  tasm::Element* element_{nullptr};
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_ANIMATION_DELEGATE_H_
