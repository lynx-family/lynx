// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

#import <Lynx/LUIBodyView.h>

namespace lynx {
namespace tasm {
PlatformRendererContextDarwin::PlatformRendererContextDarwin(UIView<LUIBodyView>* container_view) {
  _renderer_context = [[LynxRendererContext alloc] init];
  _renderer_context.bodyView = container_view;
}

PlatformRendererContextDarwin::~PlatformRendererContextDarwin() { _renderer_context = nil; }
}  // namespace tasm
}  // namespace lynx
