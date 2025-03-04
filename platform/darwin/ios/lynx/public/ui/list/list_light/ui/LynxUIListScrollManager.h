// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIListProtocol.h>

typedef NS_ENUM(NSUInteger, LynxListScrollStatus) {
  LynxListScrollStatusIdle = 0,
  LynxListScrollStatusDragging,
  LynxListScrollStatusScrolling,  // Scroll by code
  LynxListScrollStatusFling,      // Fling by touches
};

typedef NS_ENUM(NSInteger, LynxListScrollDirection) {
  LynxListScrollDirectionToLower = -1,  // scroll up
  LynxListScrollDirectionStop = 0,
  LynxListScrollDirectionToUpper = 1,  // scroll down
};

@interface LynxUIListScrollManager : NSObject <UIScrollViewDelegate>
@property(nonatomic, assign) LynxListScrollStatus scrollStatus;
@property(nonatomic, assign) LynxListScrollDirection scrollingDirection;
@property(nonatomic, assign) BOOL horizontal;
/**
 Set UISign for sending custom events.
 @param sign - LynxUI's UISign. Used to send custom events.
 */
- (void)setSign:(NSInteger)sign;

- (void)updateScrollThresholds:(LynxUIListScrollThresholds *)scrollThreSholds;

/*
 Sending list events. Now it sends scroll events and nodiff requesting events.
 */
- (void)sendCustomEvent:(NSString *)name detail:(NSDictionary *)detail;
- (void)sendScrollEvent:(UIScrollView *)scrollView;
- (void)setEventEmitter:(LynxEventEmitter *)eventEmitter;
@end
