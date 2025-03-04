// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_EVENT_LYNXTOUCHEVENT_H_
#define DARWIN_COMMON_LYNX_EVENT_LYNXTOUCHEVENT_H_

#import <CoreGraphics/CoreGraphics.h>
#import "LynxEvent.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString *const LynxEventTouchMove;
extern NSString *const LynxEventTouchStart;
extern NSString *const LynxEventTouchEnd;
extern NSString *const LynxEventTouchCancel;
extern NSString *const LynxEventTap;
extern NSString *const LynxEventLongPress;
extern NSString *const LynxEventClick;

@interface LynxTouchEvent : LynxEvent

@property(nonatomic, readonly) CGPoint clientPoint;
@property(nonatomic, readonly) CGPoint pagePoint;
@property(nonatomic, readonly) CGPoint viewPoint;
@property(nonatomic, readonly) BOOL isMultiTouch;
@property(nonatomic, readonly) NSDictionary *touchMap;
@property(nonatomic, readonly) NSMutableDictionary *uiTouchMap;
@property(nonatomic) NSMutableDictionary<NSString *, id<LynxEventTargetBase>> *activeUIMap;

- (instancetype)initWithName:(NSString *)name targetTag:(NSInteger)target;

- (instancetype)initWithName:(NSString *)name targetTag:(NSInteger)target touchPoint:(CGPoint)point;

- (instancetype)initWithName:(NSString *)name
                   targetTag:(NSInteger)tag
                 clientPoint:(CGPoint)clientPoint
                   pagePoint:(CGPoint)pagePoint
                   viewPoint:(CGPoint)viewPoint;

- (instancetype)initWithName:(NSString *)name
                   targetTag:(NSInteger)tag
                    touchMap:(NSDictionary *)touchMap;

- (instancetype)initWithName:(NSString *)name uiTouchMap:(NSMutableDictionary *)uiTouchMap;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_EVENT_LYNXTOUCHEVENT_H_
