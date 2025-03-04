// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTouchEvent.h"

NSString *const LynxEventTouchMove = @"touchmove";
NSString *const LynxEventTouchStart = @"touchstart";
NSString *const LynxEventTouchEnd = @"touchend";
NSString *const LynxEventTouchCancel = @"touchcancel";
NSString *const LynxEventTap = @"tap";
NSString *const LynxEventLongPress = @"longpress";
NSString *const LynxEventClick = @"click";

@implementation LynxTouchEvent

- (instancetype)initWithName:(NSString *)name targetTag:(NSInteger)tag {
  self = [[LynxTouchEvent alloc] initWithName:name
                                    targetTag:tag
                                  clientPoint:CGPointZero
                                    pagePoint:CGPointZero
                                    viewPoint:CGPointZero];
  return self;
}

- (instancetype)initWithName:(NSString *)name targetTag:(NSInteger)tag touchPoint:(CGPoint)point {
  return [self initWithName:name targetTag:tag clientPoint:point pagePoint:point viewPoint:point];
}

- (instancetype)initWithName:(NSString *)name
                   targetTag:(NSInteger)tag
                 clientPoint:(CGPoint)clientPoint
                   pagePoint:(CGPoint)pagePoint
                   viewPoint:(CGPoint)viewPoint {
  self = [super initWithName:name type:kTouchEvent targetSign:tag];
  if (self) {
    _clientPoint = clientPoint;
    _pagePoint = pagePoint;
    _viewPoint = viewPoint;
  }
  return self;
}

- (instancetype)initWithName:(NSString *)name
                   targetTag:(NSInteger)tag
                    touchMap:(NSDictionary *)touchMap {
  self = [self initWithName:name targetTag:tag];
  if (self) {
    _isMultiTouch = YES;
    _touchMap = touchMap;
  }
  return self;
}

- (instancetype)initWithName:(NSString *)name uiTouchMap:(NSMutableDictionary *)uiTouchMap {
  self = [self initWithName:name targetTag:-1];
  if (self) {
    _isMultiTouch = YES;
    _uiTouchMap = uiTouchMap;
  }
  return self;
}

@end
