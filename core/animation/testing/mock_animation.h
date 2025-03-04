// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TESTING_MOCK_ANIMATION_H_
#define CORE_ANIMATION_TESTING_MOCK_ANIMATION_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "base/include/fml/time/time_point.h"
#include "core/animation/animation.h"
#include "core/animation/keyframe_effect.h"

namespace lynx {

namespace animation {

class MockAnimation : public Animation {
 public:
  MockAnimation(const std::string& name) : Animation(name) {}
  ~MockAnimation() = default;
  const fml::TimePoint& start_time() { return start_time_; }
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_TESTING_MOCK_ANIMATION_H_
