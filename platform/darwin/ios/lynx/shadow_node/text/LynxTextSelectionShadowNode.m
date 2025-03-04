// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextSelectionShadowNode.h"
#import "LynxBaseTextShadowNode.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"

@implementation LynxTextSelectionShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("text-selection")
#else
LYNX_REGISTER_SHADOW_NODE("text-selection")
#endif

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self) {
    _backgroundColor = UIColor.systemBlueColor;
  }
  return self;
}

- (BOOL)isVirtual {
  return YES;
}

LYNX_PROP_SETTER("background-color", setSelectionBackground, UIColor *) {
  if (requestReset) {
    value = UIColor.systemBlueColor;
  }

  _backgroundColor = value;
}

@end
