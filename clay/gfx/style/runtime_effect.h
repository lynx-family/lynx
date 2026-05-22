// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_RUNTIME_EFFECT_H_
#define CLAY_GFX_STYLE_RUNTIME_EFFECT_H_

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class RuntimeEffect : public fml::RefCountedThreadSafe<RuntimeEffect> {
 public:
#ifndef ENABLE_SKITY
  static fml::RefPtr<RuntimeEffect> MakeSkia(
      const sk_sp<SkRuntimeEffect>& runtime_effect);

  virtual sk_sp<SkRuntimeEffect> skia_runtime_effect() const = 0;
#endif  // ENABLE_SKITY

  virtual ~RuntimeEffect();

 protected:
  RuntimeEffect();

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(RuntimeEffect);
};

#ifndef ENABLE_SKITY
class RuntimeEffectSkia final : public RuntimeEffect {
 public:
  explicit RuntimeEffectSkia(const sk_sp<SkRuntimeEffect>& runtime_effect);

  // |DlRuntimeEffect|
  sk_sp<SkRuntimeEffect> skia_runtime_effect() const override;
  ~RuntimeEffectSkia() override;

 private:
  RuntimeEffectSkia() = delete;

  sk_sp<SkRuntimeEffect> skia_runtime_effect_;

  BASE_DISALLOW_COPY_AND_ASSIGN(RuntimeEffectSkia);

  friend RuntimeEffectSkia;
};
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_GFX_STYLE_RUNTIME_EFFECT_H_
