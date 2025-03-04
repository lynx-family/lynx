// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIInlineImage.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"

@implementation LynxInlineImageShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("inline-image")
#else
LYNX_REGISTER_SHADOW_NODE("inline-image")
#endif

LYNX_PROP_SETTER("vertical-align", setVerticalAlign, NSArray *) {
  [self setVerticalAlignOnShadowNode:requestReset value:value];
}

@end

@implementation LynxUIInlineImage

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("inline-image")
#else
LYNX_REGISTER_UI("inline-image")
#endif

- (void)frameDidChange {
  [super frameDidChange];
  [self requestImage];
}

@end
