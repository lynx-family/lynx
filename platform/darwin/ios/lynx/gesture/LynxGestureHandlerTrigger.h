// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxNewGestureDelegate.h>

NS_ASSUME_NONNULL_BEGIN

static const int LYNX_STATE_INIT = 0;
static const int LYNX_STATE_BEGIN = 1;
static const int LYNX_STATE_ACTIVE = 2;
static const int LYNX_STATE_FAIL = 3;
static const int LYNX_STATE_END = 4;
static const int LYNX_STATE_UNDETERMINED = 5;

@protocol LynxGestureArenaMember;
@class LynxGestureDetectorManager;
@class LynxGestureVelocityTracker;
@class LynxGestureArenaManager;
@class LynxTouchEvent;
@class LynxEventHandler;

/**
 * This class represents a Gesture Handler Trigger that manages touch gestures and dispatches events
 * to appropriate gesture handlers. It facilitates the recognition and handling of touch events
 * and manages the state of the active gestures. The class coordinates interactions between
 * various gesture detectors and their associated handlers.
 *
 * The GestureHandlerTrigger is responsible for identifying the current winner of the touch event,
 * updating simultaneous winners, computing scrolls, and dispatching events to the respective
 * gesture handlers based on the type of gesture and event.
 *
 * The class maintains a list of GestureArenaMembers to compete with and handles the bubbling of
 * touch events to the corresponding gesture handlers.
 *
 * This class is typically used in conjunction with GestureDetectorManager to coordinate touch
 * interactions and support complex gesture handling in various applications.
 */

@interface LynxGestureHandlerTrigger : NSObject

/**
 Constructs a LynxGestureHandlerTrigger instance.
 */
- (instancetype)initWithDetectorManager:(LynxGestureDetectorManager *)detectorManager
                           arenaManager:(LynxGestureArenaManager *)arenaManager;

/**
 Initialize the current winner when a touchdown event occurs.
 @param winner The GestureArenaMember representing the current winner.
 */
- (void)setCurrentWinnerWhenDown:(id<LynxGestureArenaMember>)winner;

/**
 Resolve the touch event and dispatches appropriate events to gesture handlers.
 @param touchType The LynxEventTouchType associated with the event.
 @param touches The UITouches associated with the event.
 @param event The UIEvent to dispatch.
 @param completionChainCandidate     The linked list of GestureArenaMembers to compete with.
 @param touchEvent             The LynxTouchEvent associated with the event.
 */
- (void)resolveTouchEvent:(NSString *const)touchType
                      touches:(NSSet<UITouch *> *)touches
                        event:(UIEvent *)event
                   touchEvent:(LynxTouchEvent *)touchEvent
    completionChainCandidates:(NSArray<id<LynxGestureArenaMember>> *)completionChainCandidate
             bubbleCandidates:(NSArray<id<LynxGestureArenaMember>> *)bubbleCandidates;

/**
 Dispatch the bubble touch event to gesture handlers based on the event type and touch event.
 @param touchType The LynxEventTouchType associated with the event.
 @param touchEvent           The LynxTouchEvent associated with the event.
 @param bubbleCandidate     The linked list of GestureArenaMembers that can bubble the event.
 @param winner              The current GestureArenaMember winner.
 */
- (void)dispatchBubble:(NSString *const)touchType
            touchEvent:(LynxTouchEvent *)touchEvent
       bubbleCandidate:(NSArray<id<LynxGestureArenaMember>> *)bubbleCandidate
                winner:(id<LynxGestureArenaMember>)winner;

/**
 Handle the gesture detector state changes and performs actions accordingly.
 @param member     The GestureArenaMember associated with the state change.
 @param gestureId  The gesture ID of the gesture detector.
 @param state      The state of the gesture detector.
 */
- (void)handleGestureDetectorState:(id<LynxGestureArenaMember>)member
                         gestureId:(NSInteger)gestureId
                             state:(LynxGestureState)state;

- (NSInteger)addVelocityTracker:(LynxGestureVelocityTracker *)velocityTracker;

- (NSInteger)addEventHandler:(LynxEventHandler *)eventHandler;

- (void)removeVelocityTracker:(NSInteger)index;

- (void)removeEventHandler:(NSInteger)index;

@end

NS_ASSUME_NONNULL_END
