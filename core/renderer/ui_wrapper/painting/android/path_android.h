// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PATH_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PATH_ANDROID_H_

#include <memory>

#include "gfx/public/path_impl_factory.h"

namespace lynx::tasm {

class PathAndroid final : public gfx::PathImpl {
 public:
  explicit PathAndroid(gfx::PathSource source);
  ~PathAndroid() override;
  BackendKind GetBackendKind() const override { return BackendKind::kAndroid; }
};

class PathAndroidFactory final : public gfx::PathImplFactory {
 public:
  std::unique_ptr<gfx::PathImpl> Create(gfx::PathSource source) const override;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PATH_ANDROID_H_
