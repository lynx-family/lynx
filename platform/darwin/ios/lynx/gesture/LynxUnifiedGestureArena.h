// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxNewGestureDelegate.h>
#import <Lynx/LynxUIOwner.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxUnifiedGestureInputType) {
  LynxUnifiedGestureInputTypeDown,
  LynxUnifiedGestureInputTypeMove,
  LynxUnifiedGestureInputTypeUp,
  LynxUnifiedGestureInputTypeCancel,
};

typedef NS_OPTIONS(NSUInteger, LynxUnifiedGestureRegistration) {
  LynxUnifiedGestureRegistrationHasDetector = 1 << 0,
  LynxUnifiedGestureRegistrationHasNativeDetector = 1 << 1,
};

@interface LynxUnifiedGestureArena : NSObject

- (instancetype)initWithUIOwner:(LynxUIOwner *)uiOwner;
- (void)replaceGestureDetectors:
            (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors
                      forMember:(NSInteger)memberId;
- (void)removeMember:(NSInteger)memberId;
- (LynxUnifiedGestureRegistration)registrationForDetectors:
    (nullable NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors;
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

@interface LynxUIOwner (UnifiedGestureHandler)
@property(nonatomic, strong, nullable) LynxUnifiedGestureArena *unifiedGestureArena;
- (void)initNewGestureInUIThread:(BOOL)enableNewGesture
     enableUnifiedGestureHandler:(BOOL)enableUnifiedGestureHandler;
@end

NS_ASSUME_NONNULL_END
