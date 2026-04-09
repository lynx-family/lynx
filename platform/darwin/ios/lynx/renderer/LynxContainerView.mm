// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContainerView.h>
#import <Lynx/LynxRenderer.h>

@implementation LynxContainerView
@synthesize renderer = _renderer;
@synthesize rendererContext = _rendererContext;

- (instancetype)initWithRendererContext:(LynxRendererContext*)context {
  self = [super init];
  if (self) {
    self.rendererContext = context;
  }
  return self;
}

- (LynxRenderer*)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext*)context {
  return [[LynxRenderer alloc] initWithRenderHost:self andSign:sign andContext:context];
}

- (UIView*)view {
  return self;
}

@end
