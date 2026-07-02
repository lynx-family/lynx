// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxContainerView.h>
#import <Lynx/LynxLazyLoad.h>
#import <Lynx/LynxRenderer.h>

#import "LynxSVGRenderer.h"

@interface LynxSVGRendererHost : LynxContainerView
@end

@implementation LynxSVGRendererHost

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_RENDERER_HOST("svg")
#else
LYNX_REGISTER_RENDERER_HOST("svg")
#endif

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context {
  LynxSVGRenderer *renderer = [[LynxSVGRenderer alloc] initWithRenderHost:self
                                                                  andSign:sign
                                                               andContext:context];
  self.renderer = renderer;
  return renderer;
}

@end
