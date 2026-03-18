// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxUIViewPagerItem.h>

@interface LynxUIViewPagerItem ()

@end

@implementation LynxUIViewPagerItem

- (UIView *)createView {
  return [[UIView alloc] init];
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  // Make the item stick to left
  [super updateFrame:CGRectMake(0, frame.origin.y, frame.size.width, frame.size.height)
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];
  [self.view.superview setNeedsLayout];
}

@end
