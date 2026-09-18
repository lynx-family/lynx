// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_ANIMATION_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_ANIMATION_DARWIN_H_

#include <memory>

#include "gfx/animation/platform_animation.h"

@class LynxUI;

namespace lynx::tasm {
void ApplyPlatformAnimationCommands(
    LynxUI* ui, const std::shared_ptr<gfx::PlatformAnimationCommandBatch>& commands);
}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_ANIMATION_DARWIN_H_
