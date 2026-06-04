// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxSVGRendererHost.h>

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxLazyLoad.h>
#import <Lynx/LynxRenderer.h>

#import <XElement/LynxSVGRenderer.h>

@implementation LynxSVGRendererHost

LYNX_LAZY_REGISTER_RENDERER_HOST("svg")

- (instancetype)initWithRendererContext:(LynxRendererContext *)context {
  self = [super initWithRendererContext:context];
  if (self) {
    self.clipsToBounds = YES;
  }
  return self;
}

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context {
  LynxSVGRenderer *renderer = [[LynxSVGRenderer alloc] initWithRenderHost:self
                                                                  andSign:sign
                                                               andContext:context];
  self.renderer = renderer;
  return renderer;
}

- (void)setImage:(nullable UIImage *)image {
  self.layer.contents = (__bridge id _Nullable)(image.CGImage);
}

@end
