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
class PlatformEventBundle;
class NativePaintingContext {
 public:
  NativePaintingContext() = default;
  virtual ~NativePaintingContext() = default;
  virtual void CreatePlatformRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data) = 0;
  virtual void CreatePlatformExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data) = 0;
  virtual void UpdateDisplayList(int id, DisplayList list) = 0;
  virtual void CreateImage(int id, base::String src, float width, float height,
                           int32_t event_mask = 0) = 0;
  virtual void UpdateTextBundle(int id, intptr_t bundle) = 0;
  virtual void DestroyTextBundle(int id) = 0;
  virtual void ReconstructEventTargetTreeRecursively() = 0;
  virtual void UpdatePlatformEventBundle(int id,
                                         PlatformEventBundle bundle) = 0;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_H_
