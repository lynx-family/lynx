// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRawTextShadowNode.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"
#import "LynxTextUtils.h"

@implementation LynxRawTextShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("raw-text")
#else
LYNX_REGISTER_SHADOW_NODE("raw-text")
#endif

- (BOOL)isVirtual {
  return YES;
}

LYNX_PROP_SETTER("text", setText, id) {
  if (requestReset) {
    value = nil;
  }
  NSString *text = [LynxTextUtils ConvertRawText:value];
  if (_text != text) {
    _text = text;
    [self setNeedsLayout];
  }
}

@end
