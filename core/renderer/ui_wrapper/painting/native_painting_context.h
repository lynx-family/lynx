// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_H_

#include "base/include/value/base_string.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/public/platform_renderer_type.h"

namespace lynx::tasm {
class DisplayList;
class NativePaintingContext {
 public:
  virtual ~NativePaintingContext() = default;
  virtual void CreatePlatformRenderer(int id, PlatformRendererType type) = 0;
  virtual void UpdateDisplayList(int id, DisplayList list) = 0;
  virtual void CreateImage(int id, base::String src, float width,
                           float height) = 0;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_H_
