// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_H_

#include <memory>

#include "gfx/public/path_impl_factory.h"

namespace lynx::tasm {

// Source-only backend skeleton; native access belongs to a later consumer
// slice.
class PathHarmony final : public gfx::PathImpl {
 public:
  explicit PathHarmony(gfx::PathSource source);
  ~PathHarmony() override;
};

class PathHarmonyFactory final : public gfx::PathImplFactory {
 public:
  std::unique_ptr<gfx::PathImpl> Create(gfx::PathSource source) const override;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_PATH_HARMONY_H_
