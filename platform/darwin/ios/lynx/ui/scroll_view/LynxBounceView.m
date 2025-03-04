// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBounceView.h"
#import <objc/runtime.h>
#import "LynxComponentRegistry.h"
#import "LynxLayoutStyle.h"
#import "LynxPropsProcessor.h"
#import "LynxView.h"

@implementation LynxBounceView

- (instancetype)init {
  self = [super init];
  return self;
}

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("bounce-view")
#else
LYNX_REGISTER_UI("bounce-view")
#endif

LYNX_PROP_SETTER("direction", direction, NSString *) {
  if ([value isEqualToString:@"right"]) {
    _direction = LynxBounceViewDirectionRight;
  } else if ([value isEqualToString:@"bottom"]) {
    _direction = LynxBounceViewDirectionBottom;
  } else if ([value isEqualToString:@"left"]) {
    _direction = LynxBounceViewDirectionLeft;
  } else if ([value isEqualToString:@"top"]) {
    _direction = LynxBounceViewDirectionTop;
  }
}

LYNX_PROP_SETTER("bounce-trigger-distance", setTriggerBounceEventDistance, CGFloat) {
  _triggerBounceEventDistance = value;
}
@end
