// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxUIScrollCoordinatorSlotDrag.h>

@implementation LynxUIScrollCoordinatorSlotDrag

- (UIView *)createView {
  return [[UIView alloc] init];
}

- (BOOL)notifyParent {
  return YES;
}

#pragma mark - LYNX_PROPS

LYNX_PROP_SETTER("enable-drag", setEnableDrag, BOOL) { self.forbidMovable = !value; }

@end
