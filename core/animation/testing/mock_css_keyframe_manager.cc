// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/testing/mock_css_keyframe_manager.h"

#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/animation/animation.h"
#include "core/animation/animation_delegate.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/utils/timing_function.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace animation {
void MockCSSKeyframeManager::ClearUTStatus() {
  has_flush_animated_style_ = false;
  has_request_next_frame_ = false;
}
}  // namespace animation
}  // namespace lynx
