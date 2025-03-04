// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextOverflowLayer.h"
#import "LynxTextView.h"
#import "LynxUIText.h"
#import "LynxUIUnitUtils.h"

@implementation LynxTextOverflowLayer

- (instancetype)init {
  return [self initWithView:nil];
}

- (instancetype)initWithView:(LynxTextView*)view {
  self = [super init];
  self.drawsAsynchronously = YES;
  self.contentsScale = [LynxUIUnitUtils screenScale];
  _view = view;
  _view.clipsToBounds = NO;
  return self;
}

- (void)drawInContext:(CGContextRef)ctx {
  __strong LynxTextView* strongView = self.view;
  if (strongView) {
    UIGraphicsPushContext(ctx);
    CGRect bounds = {.origin = CGPointMake(self.view.ui.overflowLayerOffset.x,
                                           self.view.ui.overflowLayerOffset.y),
                     .size = self.view.ui.frame.size};
    [_view.ui.renderer drawRect:bounds padding:self.view.ui.padding border:self.view.ui.border];
    UIGraphicsPopContext();
  }
}

@end
