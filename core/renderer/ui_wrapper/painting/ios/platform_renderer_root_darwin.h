// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_ROOT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_ROOT_DARWIN_H_

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h"

namespace lynx {
namespace tasm {

class PlatformRendererRootDarwin : public PlatformRendererDarwin {
 public:
  explicit PlatformRendererRootDarwin(PlatformRendererContextDarwin* context, int id,
                                      PlatformRendererType type);
  ~PlatformRendererRootDarwin() override = default;

  UIView<LynxRendererHost>* GetUIView() override;

 private:
  __weak UIView<LynxRendererHost>* _rootView = nil;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PLATFORM_RENDERER_ROOT_DARWIN_H_
