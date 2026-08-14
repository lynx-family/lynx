// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUIOwner.h>

namespace lynx {
namespace tasm {
PlatformRendererContextDarwin::PlatformRendererContextDarwin(
    UIView<LUIBodyView>* container_view, LynxUIOwner* ui_owner,
    LynxComponentScopeRegistry* component_registry)
    : ui_owner_(ui_owner), component_registry_(component_registry) {
  renderer_context_ = [[LynxRendererContext alloc] init];
  renderer_context_.bodyView = container_view;
}

PlatformRendererContextDarwin::~PlatformRendererContextDarwin() {
  renderer_context_ = nil;
  ui_owner_ = nil;
  component_registry_ = nil;
}

CGPoint PlatformRendererContextDarwin::GetRootViewLocationOnScreen() {
  UIView<LUIBodyView>* view = GetContainerView();
  if (view == nil) {
    return CGPointZero;
  }
  return [view convertPoint:CGPointZero toView:nil];
}

CGSize PlatformRendererContextDarwin::GetScreenSize() {
  // TODO(xiamengfei.moonface): [ResizableWindowSize] Check whether this should use window or
  // physical screen metrics.
  CGSize size = UIScreen.mainScreen.bounds.size;
  return size;
}
}  // namespace tasm
}  // namespace lynx
