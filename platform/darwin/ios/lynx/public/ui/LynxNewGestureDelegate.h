// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxGestureState) {
  LynxGestureStateActive =
      1,                     // When the gesture wins, it wins the gesture, and handles the gesture.
  LynxGestureStateFail = 2,  // When the gesture fails, it will abandon the current gesture and the
                             // gesture response will be transferred to the next competitor.
  LynxGestureStateEnd = 3  // When the gesture ends, the current gesture response will be terminated
                           // directly, and the re-competition logic will not be executed.
};

@protocol LynxNewGestureDelegate <NSObject>

@optional
/**
 * Set the state of the gesture detector to the specified state.
 *
 * @param gestureId The ID of the gesture detector.
 * @param state     An integer value representing the state to set the gesture detector to.
 *                  Must be one of the following values: STATE_ACTIVE, STATE_FAIL, or STATE_END.
 * @note It is important to implement this method in any class that conforms to the
 * LynxNewGestureDelegate protocol to ensure the proper functioning of the gesture detector.
 */
- (void)setGestureDetectorState:(NSInteger)gestureId state:(LynxGestureState)state;

/**
 * consume gesture in lynxView internal or outside
 *
 * @param gestureId The ID of the gesture detector.
 * @param params The params
 */
- (void)consumeGesture:(NSInteger)gestureId params:(NSDictionary *)params;

/**
 * Scrolls the content by the specified amount in the x and y directions.
 *
 * @param deltaX The amount to scroll in the x direction.
 * @param deltaY The amount to scroll in the y direction.
 *
 * @return An array containing the new x and y positions of the content after scrolling.
 */
- (NSArray<NSNumber *> *)scrollBy:(CGFloat)deltaX deltaY:(CGFloat)deltaY;

@end

NS_ASSUME_NONNULL_END
