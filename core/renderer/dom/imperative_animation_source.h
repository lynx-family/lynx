// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_SOURCE_H_
#define CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_SOURCE_H_

#include <cstdint>

namespace lynx {
namespace tasm {

// Identifies the Lynx API entry point that owns an imperative animation.
// Both values map to Animation::Origin::kWebAnimation, but remain distinct
// here because Animate and AnimateV2 use different replacement rules.
enum class ImperativeAnimationSource : uint8_t {
  // Native Element.animate(): a new start replaces the previous Animate entry.
  kAnimate,
  // SelectorQuery AnimateV2: entries are independently addressed by identity.
  kAnimateV2,
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_IMPERATIVE_ANIMATION_SOURCE_H_
