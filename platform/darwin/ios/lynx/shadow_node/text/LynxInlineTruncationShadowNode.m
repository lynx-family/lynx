// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxInlineTruncationShadowNode.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"

@implementation LynxInlineTruncationShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("inline-truncation")
#else
LYNX_REGISTER_SHADOW_NODE("inline-truncation")
#endif

- (BOOL)isVirtual {
  return YES;
}

- (BOOL)needsEventSet {
  return YES;
}

@end
