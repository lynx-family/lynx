// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContainerView.h>
#import <Lynx/LynxRenderer.h>

@implementation LynxContainerView {
  LynxRenderer* _render;
}

- (void)setRenderer:(LynxRenderer*)renderer {
  _render = renderer;
}

- (LynxRenderer*)createRendererWithSign:(int32_t)sign {
  return [[LynxRenderer alloc] initWithRenderHost:self andSign:sign];
}

- (LynxRenderer*)getRenderer {
  return _render;
}

- (UIView*)getView {
  return self;
}

@end
