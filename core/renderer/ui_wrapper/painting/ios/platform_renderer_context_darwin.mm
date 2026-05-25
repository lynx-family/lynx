// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxUIOwner.h>

namespace lynx {
namespace tasm {
PlatformRendererContextDarwin::PlatformRendererContextDarwin(UIView<LUIBodyView>* container_view,
                                                             LynxUIOwner* ui_owner)
    : _ui_owner(ui_owner) {
  _renderer_context = [[LynxRendererContext alloc] init];
  _renderer_context.bodyView = container_view;
}

PlatformRendererContextDarwin::~PlatformRendererContextDarwin() {
  _renderer_context = nil;
  _ui_owner = nil;
}

CGPoint PlatformRendererContextDarwin::GetRootViewLocationOnScreen() {
  UIView<LUIBodyView>* view = GetContainerView();
  if (view == nil) {
    return CGPointZero;
  }
  return [view convertPoint:CGPointZero toView:nil];
}

CGSize PlatformRendererContextDarwin::GetScreenSize() {
  CGSize size = UIScreen.mainScreen.bounds.size;
  return size;
}
}  // namespace tasm
}  // namespace lynx
