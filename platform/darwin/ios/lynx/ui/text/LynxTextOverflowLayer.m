// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTextOverflowLayer.h>
#import <Lynx/LynxTextView.h>
#import <Lynx/LynxUIText.h>
#import <Lynx/LynxUIUnitUtils.h>

@implementation LynxTextOverflowLayer

- (instancetype)init {
  return [self initWithView:nil];
}

- (instancetype)initWithView:(LynxTextView*)view {
  self = [super init];
  self.contentsScale = [LynxUIUnitUtils screenScale];
  _view = view;
  _view.clipsToBounds = NO;
  return self;
}

- (void)drawInContext:(CGContextRef)ctx {
  __strong LynxTextView* strongView = self.view;
  if (strongView) {
    UIGraphicsPushContext(ctx);
    CGRect bounds = {.origin = CGPointMake(-self.frame.origin.x, -self.frame.origin.y),
                     .size = self.frame.size};
    [_view.ui.renderer drawRect:bounds padding:self.view.ui.padding border:self.view.ui.border];
    UIGraphicsPopContext();
  }
}

@end
