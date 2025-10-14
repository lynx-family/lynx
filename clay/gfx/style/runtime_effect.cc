// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/runtime_effect.h"

namespace clay {

//------------------------------------------------------------------------------
/// RuntimeEffect
///

RuntimeEffect::RuntimeEffect() = default;
RuntimeEffect::~RuntimeEffect() = default;

#ifndef ENABLE_SKITY
fml::RefPtr<RuntimeEffect> RuntimeEffect::MakeSkia(
    const sk_sp<SkRuntimeEffect>& runtime_effect) {
  return fml::MakeRefCounted<RuntimeEffectSkia>(runtime_effect);
}

//------------------------------------------------------------------------------
/// RuntimeEffectSkia
///

RuntimeEffectSkia::~RuntimeEffectSkia() = default;

RuntimeEffectSkia::RuntimeEffectSkia(
    const sk_sp<SkRuntimeEffect>& runtime_effect)
    : skia_runtime_effect_(runtime_effect) {}

sk_sp<SkRuntimeEffect> RuntimeEffectSkia::skia_runtime_effect() const {
  return skia_runtime_effect_;
}
#endif  // ENABLE_SKITY

}  // namespace clay
