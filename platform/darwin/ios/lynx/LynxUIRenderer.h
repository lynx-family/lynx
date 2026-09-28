// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIRendererProtocol.h>

// Keep in sync with native PlatformEventBehavior.
typedef NS_OPTIONS(NSUInteger, LynxPlatformEventBehavior) {
  LynxPlatformEventBehaviorNone = 0,
  LynxPlatformEventBehaviorIgnoreFocus = 1 << 0,
  LynxPlatformEventBehaviorEventThrough = 1 << 1,
  LynxPlatformEventBehaviorBlockNativeEvent = 1 << 2,
  LynxPlatformEventBehaviorEnableSimultaneousTouch = 1 << 3,
  LynxPlatformEventBehaviorHasConsumeSlideEvent = 1 << 4,
};

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIRenderer : NSObject <LynxUIRendererProtocol>

- (BOOL)DispatchPlatformInputEvent:(NSArray*)iEventData withData:(NSArray*)fEventData;
- (nullable LynxUI*)platformTouchTarget;
- (void)DispatchPlatformLongPress;
- (void)DispatchPlatformTap;
- (void)SetPlatformEventRootActive:(NSInteger)rootSign active:(BOOL)active;
- (void)SetPlatformEventRootOffset:(NSInteger)rootSign
                           offsetX:(CGFloat)offsetX
                           offsetY:(CGFloat)offsetY;
- (LynxPlatformEventBehavior)HitTestAndCachePlatformEventBehavior:(NSInteger)rootSign
                                                            point:(CGPoint)point;
- (NSArray<NSNumber*>*)cachedConsumeSlideEventAngles;

@end

NS_ASSUME_NONNULL_END
