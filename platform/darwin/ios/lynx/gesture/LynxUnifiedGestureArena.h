// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxNewGestureDelegate.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUIOwner;

typedef NS_ENUM(NSInteger, LynxUnifiedGestureInputType) {
  LynxUnifiedGestureInputTypeDown,
  LynxUnifiedGestureInputTypeMove,
  LynxUnifiedGestureInputTypeUp,
  LynxUnifiedGestureInputTypeCancel,
};

@interface LynxUnifiedGestureArena : NSObject

- (instancetype)initWithUIOwner:(LynxUIOwner *)uiOwner;
- (void)replaceGestureDetectors:
            (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors
                      forMember:(NSInteger)memberId;
- (void)removeMember:(NSInteger)memberId;
- (BOOL)containsMember:(NSInteger)memberId;
- (BOOL)containsGesture:(NSInteger)gestureId memberId:(NSInteger)memberId;
- (void)setGestureDetectorState:(NSInteger)gestureId
                       memberId:(NSInteger)memberId
                          state:(LynxGestureState)state;
- (void)handleTouch:(nullable UITouch *)touch
             action:(LynxUnifiedGestureInputType)action
             target:(nullable id<LynxEventTarget>)target
          pointerId:(NSInteger)pointerId
           velocity:(CGPoint)velocity;
- (BOOL)hasActiveGesture;
- (void)invalidate;

@end

NS_ASSUME_NONNULL_END
