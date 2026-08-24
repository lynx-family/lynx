// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_SCROLL_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_SCROLL_DARWIN_H_

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_scroll.h"

namespace lynx {
namespace tasm {

class PlatformRendererContextDarwin;
class PropBundle;

class PlatformRendererScrollDarwin : public PlatformRendererDarwin,
                                     public PlatformRendererScroll {
 public:
  explicit PlatformRendererScrollDarwin(
      PlatformRendererContextDarwin* context, int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data);
  ~PlatformRendererScrollDarwin() override;

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_SCROLL_DARWIN_H_
