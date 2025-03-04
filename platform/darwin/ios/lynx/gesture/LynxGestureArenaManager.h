// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxNewGestureDelegate.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxEventTarget;
@protocol LynxGestureArenaMember;
@class LynxGestureDetectorDarwin;
@class LynxGestureHandlerTrigger;
@class LynxTouchEvent;
@class LynxEventHandler;

@interface LynxGestureArenaManager : NSObject

@property(nonatomic, strong, readonly) LynxGestureHandlerTrigger *gestureHandlerTrigger;

- (NSArray<id<LynxGestureArenaMember>> *)getCompetitionChainCandidates;

/**
 Dispatch the touch event to the appropriate gesture arena member.
 @param touchType The LynxEventTouchType associated with the event.
 @param touches The UITouches associated with the event.
 @param event The UIEvent to dispatch.
 */
- (void)dispatchTouchToArena:(NSString *const)touchType
                     touches:(NSSet<UITouch *> *)touches
                       event:(UIEvent *)event
                  touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

/**
 Dispatch the bubble touch event to the appropriate gesture arena member.
 @param touchType The LynxEventTouchType associated with the event.
 @param touchEvent The LynxTouchEvent associated with the event.
 */
- (void)dispatchBubble:(NSString *const)touchType touchEvent:(LynxTouchEvent *_Nullable)touchEvent;

/**
 Set the active UI member of the arena when a down event occurs.
 @param target The LynxEventTarget associated with the active UI member.
 */
- (void)setActiveUIToArena:(id<LynxEventTarget>)target;

/**
 Add a gesture member to the arena.
 @param member The GestureArenaMember to add.
 @return The assigned member ID.
 */
- (NSInteger)addMember:(id<LynxGestureArenaMember>)member;

/**
 Check if a gesture member with the given member ID exists in the arena.
 @param memberId The member ID to check.
 @return True if the member exists, false otherwise.
 */
- (BOOL)isMemberExist:(NSInteger)memberId;

/**
 Retrieve the gesture arena member with the given ID.
 @param memberId The ID of the member.
 @return The GestureArenaMember with the given ID.
 */
- (id<LynxGestureArenaMember>)getMemberById:(NSInteger)memberId;

/**
 Remove a gesture member from the arena.
 @param member The GestureArenaMember to remove.
 @param detectorMap The map of gesture detectors associated with the member.
 */
- (void)removeMember:(id<LynxGestureArenaMember>)member
         detectorMap:(NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)detectorMap;

/**
 Register gesture detectors for the given member ID.
 @param memberId The member ID to register the detectors for.
 @param gestureDetectors The map of gesture detectors to register.
 */
- (void)registerGestureDetectors:(NSInteger)memberId
                     detectorMap:
                         (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors;

/**
 Unregister gesture detectors for the given member ID.
 @param memberId The member ID to unregister the detectors for.
 @param gestureDetectors The map of gesture detectors to unregister.
 */
- (void)unregisterGestureDetectors:(NSInteger)memberId
                       detectorMap:(NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)
                                       gestureDetectors;

/**
 Set the state of the gesture detector associated with the given member ID.
 @param memberId The member ID of the gesture detector.
 @param gestureId The ID of the gesture.
 @param state The state of the gesture.
 */
- (void)setGestureDetectorState:(NSInteger)gestureId
                       memberId:(NSInteger)memberId
                          state:(LynxGestureState)state;

@end

NS_ASSUME_NONNULL_END
