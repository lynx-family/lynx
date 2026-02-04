// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxSizeValue.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxViewInternal.h>

@implementation LynxRootUI {
  __weak id<LynxEventTarget> _parentLynxPageUI;
  NSMutableDictionary *_childrenLynxPageUI;
}

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

- (BOOL)eventThrough:(CGPoint)point {
  BOOL isEventThrough = [super eventThrough:point];
  if (!isEventThrough) {
    isEventThrough |= self.context.enableEventThrough;
  }

  if (!self.eventThroughActiveRegions) {
    return isEventThrough;
  }

  CGSize size = self.view.bounds.size;
  __block BOOL isHitEventThroughActiveRegions = NO;
  [self.eventThroughActiveRegions
      enumerateObjectsUsingBlock:^(NSArray<LynxSizeValue *> *_Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        if ([obj count] == 4) {
          CGFloat left = [obj[0] convertToDevicePtWithFullSize:size.width];
          CGFloat top = [obj[1] convertToDevicePtWithFullSize:size.height];
          CGFloat right = left + [obj[2] convertToDevicePtWithFullSize:size.width];
          CGFloat bottom = top + [obj[3] convertToDevicePtWithFullSize:size.height];
          isHitEventThroughActiveRegions =
              point.x >= left && point.x < right && point.y >= top && point.y < bottom;
          if (isHitEventThroughActiveRegions) {
            LLogInfo(@"hit the event through active regions!");
            *stop = YES;
          }
        }
      }];
  return isHitEventThroughActiveRegions ? isEventThrough : !isEventThrough;
}

- (id<LynxEventTarget>)parentLynxPageUI {
  return _parentLynxPageUI;
}

- (void)setParentLynxPageUI:(id<LynxEventTarget>)ui {
  _parentLynxPageUI = ui;
}

- (NSMutableDictionary *)childrenLynxPageUI {
  return _childrenLynxPageUI;
}

- (void)setChildrenLynxPageUI:(NSMutableDictionary *)dict {
  _childrenLynxPageUI = dict;
}

@end
