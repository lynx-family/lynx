// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIRefreshHeader.h>

@implementation LynxUIRefreshHeader

- (UIView *)createView {
  UIView *view = [UIView new];
  return view;
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent *)event {
  // The actual frame of the header view is (0,-h,w,h), and the frame in lynxUI is (0,0,w,h), so a
  // conversion is made to the y value here to prevent the click event from being intercepted by
  // mistake
  CGPoint fp = CGPointMake(point.x, point.y + self.view.frame.size.height);
  if ([self.view pointInside:fp withEvent:event]) {
    return true;
  }
  return false;
}

@end
