// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_H_

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace lynx {

namespace tasm {

// iOS specific implementation of PlatformRendererImpl.
class PlatformRendererDarwin : public PlatformRendererImpl {
 public:
  explicit PlatformRendererDarwin(int id, PlatformRendererType type);
  explicit PlatformRendererDarwin(int id, const base::String& tag_name);
  explicit PlatformRendererDarwin(int id, PlatformRendererType type,
                                  const base::String& tag_name);

  ~PlatformRendererDarwin() override = default;

  // PlatformRendererImpl interface
  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>& attributes,
                          bool tends_to_flatten) override;
  void OnAddChild(PlatformRenderer* child) override;
  void OnRemoveFromParent() override;
};

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_DARWIN_H_
