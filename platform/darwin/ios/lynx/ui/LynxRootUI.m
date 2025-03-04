// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRootUI.h"
#import "LUIBodyView.h"
#import "LynxLog.h"
#import "LynxViewInternal.h"

@implementation LynxRootUI

- (instancetype)initWithLynxView:(UIView<LUIBodyView> *)lynxView {
  NSAssert(lynxView != nil, @"LynxRootUI can not be created with nil lynxView.");
  if (self = [super initWithView:nil]) {
    if ([lynxView isKindOfClass:[LynxView class]]) {
      _lynxView = (LynxView *)lynxView;
    }
    _rootView = lynxView;
    _layoutAnimationRunning = YES;
  }
  return self;
}

- (UIView *)createView {
  return nil;
}

- (UIView *)view {
  return _rootView;
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  CGRect preFrame = _rootView.frame;
  frame.origin.x = preFrame.origin.x;
  frame.origin.y = preFrame.origin.y;
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with && _layoutAnimationRunning];
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
    withLayoutAnimation:(BOOL)with {
  [self updateFrame:frame
              withPadding:padding
                   border:border
                   margin:UIEdgeInsetsZero
      withLayoutAnimation:with];
}

- (void)onAnimationStart:(NSString *)type
              startFrame:(CGRect)startFrame
              finalFrame:(CGRect)finalFrame
                duration:(NSTimeInterval)duration {
  __strong UIView *view = _rootView;
  if (view == nil) {
    LLogError(@"LynxView is nil when LynxRootUI onAnimationStart.");
    return;
  }
  NSDictionary *dict = @{
    @"type" : type,
    @"frame" : [NSValue valueWithCGRect:startFrame],
    @"startFrame" : [NSValue valueWithCGRect:startFrame],
    @"finalFrame" : [NSValue valueWithCGRect:finalFrame],
    @"duration" : [NSNumber numberWithDouble:duration],
    @"lynxview" : view
  };
  [[NSNotificationCenter defaultCenter] postNotificationName:@"lynx_view_layout_animation_start"
                                                      object:nil
                                                    userInfo:dict];
}

- (void)onAnimationEnd:(NSString *)type
            startFrame:(CGRect)startFrame
            finalFrame:(CGRect)finalFrame
              duration:(NSTimeInterval)duration {
  __strong UIView<LUIBodyView> *view = _rootView;
  if (view == nil) {
    LLogError(@"LynxView is nil when LynxRootUI onAnimationEnd.");
    return;
  }
  view.intrinsicContentSize = finalFrame.size;
  NSDictionary *dict = @{
    @"type" : type,
    @"frame" : [NSValue valueWithCGRect:finalFrame],
    @"startFrame" : [NSValue valueWithCGRect:startFrame],
    @"finalFrame" : [NSValue valueWithCGRect:finalFrame],
    @"duration" : [NSNumber numberWithDouble:duration],
    @"lynxview" : view
  };
  [[NSNotificationCenter defaultCenter] postNotificationName:@"lynx_view_layout_animation_end"
                                                      object:nil
                                                    userInfo:dict];
}

- (BOOL)eventThrough {
  BOOL res = [super eventThrough];
  if (!res) {
    res |= self.context.enableEventThrough;
  }
  return res;
}

@end
