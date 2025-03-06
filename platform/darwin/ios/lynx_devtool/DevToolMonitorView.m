// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/DevToolMonitorView.h>

@implementation DevToolMonitorView

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  UIView* view = [super hitTest:point withEvent:event];
  if (view == self) {
    return nil;
  }
  return view;
}

@end
