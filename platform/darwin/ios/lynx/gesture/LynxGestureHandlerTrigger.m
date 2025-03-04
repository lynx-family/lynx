// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGestureHandlerTrigger.h"
#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxTouchEvent.h>
#import "LynxGestureArenaManager.h"
#import "LynxGestureDetectorManager.h"
#import "LynxGestureFlingTrigger.h"

@interface LynxGestureHandlerTrigger ()

@property(nonatomic, strong) NSPointerArray *velocityTrackers;
@property(nonatomic, strong) NSPointerArray *eventHandlers;

@property(nonatomic, weak) LynxGestureVelocityTracker *velocityTracker;
@property(nonatomic, weak) LynxEventHandler *eventHandler;

@property(nonatomic, strong) LynxGestureFlingTrigger *flinger;

@property(nonatomic, weak) LynxGestureDetectorManager *gestureDetectorManager;
@property(nonatomic, weak) LynxGestureArenaManager *gestureArenaManager;
@property(nonatomic, weak) id<LynxGestureArenaMember> winner;
@property(nonatomic, weak) id<LynxGestureArenaMember> lastWinner;
@property(nonatomic, weak) id<LynxGestureArenaMember> lastFlingWinner;
@property(nonatomic, strong) NSSet<id<LynxGestureArenaMember>> *simultaneousWinners;
@property(nonatomic, strong) NSSet<NSNumber *> *simultaneousGestureIds;

@property(nonatomic, assign) NSInteger lastFlingTargetId;
// record duplicated member when use continuesWith, such as A -> B -> A -> C
@property(nonatomic, weak) id<LynxGestureArenaMember> duplicatedMember;

@end

@implementation LynxGestureHandlerTrigger

- (instancetype)initWithDetectorManager:(LynxGestureDetectorManager *)detectorManager
                           arenaManager:(LynxGestureArenaManager *)arenaManager {
  if (self = [super init]) {
    _gestureDetectorManager = detectorManager;
    _flinger = [[LynxGestureFlingTrigger alloc] initWithTarget:self action:@selector(handleFling:)];
    _gestureArenaManager = arenaManager;
    _velocityTrackers = [NSPointerArray weakObjectsPointerArray];
    _eventHandlers = [NSPointerArray weakObjectsPointerArray];
  }
  return self;
}

- (NSInteger)addVelocityTracker:(LynxGestureVelocityTracker *)velocityTracker {
  [self.velocityTrackers addPointer:(__bridge void *_Nullable)(velocityTracker)];
  return _velocityTrackers.count - 1;
}

- (NSInteger)addEventHandler:(LynxEventHandler *)eventHandler {
  [self.eventHandlers addPointer:(__bridge void *_Nullable)(eventHandler)];
  return _eventHandlers.count - 1;
}

- (void)removeVelocityTracker:(NSInteger)index {
  if (index >= 0 && index < (NSInteger)_velocityTrackers.count) {
    [self.velocityTrackers removePointerAtIndex:index];
  }
}

- (void)removeEventHandler:(NSInteger)index {
  if (index >= 0 && index < (NSInteger)_eventHandlers.count) {
    [self.eventHandlers removePointerAtIndex:index];
  }
}

- (NSInteger)getLastNotNullIndexOfNSPointer:(NSPointerArray *)array {
  if (array.count <= 0) {
    return -1;
  }

  bool needCompat = false;
  for (NSInteger i = array.count - 1; i >= 0; i--) {
    if ([array pointerAtIndex:i] != nil) {
      if (needCompat) {
        [array compact];
      }
      return i;
    } else {
      needCompat = true;
    }
  }
  if (needCompat) {
    [array compact];
  }
  return -1;
}

- (void)setCurrentWinnerWhenDown:(id<LynxGestureArenaMember>)winner {
  _winner = winner;
  _lastWinner = winner;
  [self updateSimultaneousWinner:winner];
}

- (void)updateSimultaneousWinner:(id<LynxGestureArenaMember>)winner {
  NSDictionary<NSString *, NSSet *> *result =
      [_gestureDetectorManager handleSimultaneousWinner:winner];
  _simultaneousWinners = [result objectForKey:@"LynxGestureArenaMembers"];
  _simultaneousGestureIds = [result objectForKey:@"SimultaneousGestureIds"];
}

- (void)resolveTouchEvent:(NSString *const)touchType
                      touches:(NSSet<UITouch *> *)touches
                        event:(UIEvent *)event
                   touchEvent:(LynxTouchEvent *)touchEvent
    completionChainCandidates:(NSArray<id<LynxGestureArenaMember>> *)competeChainCandidates
             bubbleCandidates:(NSArray<id<LynxGestureArenaMember>> *)bubbleCandidates {
  CGPoint touchPoint = [[touches anyObject] locationInView:nil];

  if (touchType == LynxEventTouchStart) {
    [self resetCandidatesGestures:(competeChainCandidates)];

    [self stopFlingByLastFlingMember:touchType
                    bubbleCandidates:bubbleCandidates
              competeChainCandidates:competeChainCandidates
                             touches:touches
                               event:event
                          touchEvent:touchEvent];

    [self dispatchUIEventWithSimultaneousAndReCompete:touchType
                                              touches:touches
                                                event:event
                                           touchEvent:touchEvent
                                               winner:_winner
                                                delta:touchPoint
                               competeChainCandidates:competeChainCandidates];

    [self findNextWinnerInBegin:touchType
                        touches:touches
                          event:event
                     touchEvent:touchEvent
                          delta:touchPoint
         competeChainCandidates:competeChainCandidates];

  } else if (touchType == LynxEventTouchMove) {
    _winner = [self reCompeteByGestures:competeChainCandidates current:_winner];
    if (_winner == _lastWinner) {
      [self dispatchUIEventWithSimultaneous:touchType
                                    touches:touches
                                      event:event
                                 touchEvent:touchEvent
                                     winner:_winner
                                      delta:touchPoint];
    }

    [self findNextWinnerInBegin:touchType
                        touches:touches
                          event:event
                     touchEvent:touchEvent
                          delta:touchPoint
         competeChainCandidates:competeChainCandidates];

  } else if (touchType == LynxEventTouchEnd || touchType == LynxEventTouchCancel) {
    [self dispatchUIEventWithSimultaneousAndReCompete:touchType
                                              touches:touches
                                                event:event
                                           touchEvent:touchEvent
                                               winner:_winner
                                                delta:touchPoint
                               competeChainCandidates:competeChainCandidates];

    // get the velocity & fling

    NSInteger lastIndex = [self getLastNotNullIndexOfNSPointer:_velocityTrackers];
    if (lastIndex >= 0) {
      _velocityTracker = [_velocityTrackers pointerAtIndex:lastIndex];
    }
    CGPoint velocity = [_velocityTracker velocityInView:nil];

    if (_winner && [_flinger startWithVelocity:velocity]) {
      // trigger fling
      _lastFlingWinner = _lastWinner;
    } else {
      [_flinger reset];
      [self dispatchUIEventWithSimultaneousAndReCompete:touchType
                                                touches:nil
                                                  event:nil
                                             touchEvent:nil
                                                 winner:_winner
                                                  delta:CGPointMake(FLT_EPSILON, FLT_EPSILON)
                                 competeChainCandidates:competeChainCandidates];
      if (_winner) {
        [self handleFling:_flinger];
      }
    }
  }
}

- (void)dispatchBubble:(NSString *const)touchType
            touchEvent:(LynxTouchEvent *)touchEvent
       bubbleCandidate:(NSArray<id<LynxGestureArenaMember>> *)bubbleCandidate
                winner:(id<LynxGestureArenaMember>)winner {
  if (!_winner || (touchType != LynxEventTouchStart && touchType != LynxEventTouchMove &&
                   touchType != LynxEventTouchCancel && touchType != LynxEventTouchEnd)) {
    return;
  }

  for (NSUInteger i = 0; i < bubbleCandidate.count; i++) {
    NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers =
        [bubbleCandidate[i] getGestureHandlers];

    [gestureHandlers
        enumerateKeysAndObjectsUsingBlock:^(
            NSNumber *_Nonnull key, LynxBaseGestureHandler *_Nonnull handler, BOOL *_Nonnull stop) {
          if (touchType == LynxEventTouchStart) {
            [handler onTouchesDown:touchEvent];
          } else if (touchType == LynxEventTouchMove) {
            [handler onTouchesMove:touchEvent];
          } else if (touchType == LynxEventTouchEnd) {
            [handler onTouchesUp:touchEvent];
          } else if (touchType == LynxEventTouchCancel) {
            [handler onTouchesCancel:touchEvent];
          }
        }];
  }
}

/**
 Determines the new winner among the competitor chain based on the current delta values.
 @param competitionChainCandidates  The linked list of GestureArenaMembers to compete with.
 @param current The current GestureArenaMember winner.
 @return The new GestureArenaMember winner.
 */
- (id<LynxGestureArenaMember>)reCompeteByGestures:
                                  (NSArray<id<LynxGestureArenaMember>> *)competitionChainCandidates
                                          current:(id<LynxGestureArenaMember>)current {
  if ((!current && !_lastWinner) || !competitionChainCandidates) {
    return nil;
  }

  bool needReCompeteLastWinner = NO;
  if (!current && _lastWinner) {
    current = _lastWinner;
    needReCompeteLastWinner = YES;
    [self resetGestureHandlerAndSimultaneous:_lastWinner];
  }

  int stateCurrent = [self getCurrentMemberState:current];

  if (stateCurrent <= LYNX_STATE_ACTIVE) {
    return current;
  } else if (stateCurrent == LYNX_STATE_END) {
    return nil;
  }

  if (needReCompeteLastWinner) {
    return nil;
  }

  NSUInteger index = [competitionChainCandidates indexOfObject:current];
  if (index == NSNotFound) {
    return nil;
  }
  NSUInteger lastIndex = [competitionChainCandidates
      indexOfObject:current
            inRange:NSMakeRange(index + 1, competitionChainCandidates.count - index - 1)];

  if (index != NSNotFound && lastIndex != NSNotFound && index != lastIndex) {
    if (_duplicatedMember != nil) {
      index = lastIndex;
      [self resetGestureHandlerAndSimultaneous:_duplicatedMember];
    } else {
      _duplicatedMember = competitionChainCandidates[index];
    }
  }

  if (index >= competitionChainCandidates.count || index < 0) {
    return nil;
  }

  NSInteger currentMemberId = [competitionChainCandidates[index] getGestureArenaMemberId];

  // Ignore the same candidates id, such as A -> B -> B -> C, if B is failed, need to judge C
  // Whether the conditions are met reCompete to end of competitor chain
  for (NSUInteger i = index + 1; i < competitionChainCandidates.count; i++) {
    id<LynxGestureArenaMember> node = competitionChainCandidates[i];
    if ([node getGestureArenaMemberId] == currentMemberId) {
      continue;
    }

    // reset gesture handler to init status before getCurrentMemberState

    if (_duplicatedMember == node) {
      _duplicatedMember = nil;
    } else {
      [self resetGestureHandlerAndSimultaneous:node];
    }

    int state = [self getCurrentMemberState:node];
    if (state <= LYNX_STATE_ACTIVE) {
      return node;
    } else if (state == LYNX_STATE_END) {
      return nil;
    }
  }

  for (NSUInteger i = 0; i < index; i++) {
    // reCompete from i to start of competitor chain
    id<LynxGestureArenaMember> node = competitionChainCandidates[i];
    if ([node getGestureArenaMemberId] == currentMemberId) {
      continue;
    }

    // reset gesture handler to init status before getCurrentMemberState
    if (_duplicatedMember == node) {
      _duplicatedMember = nil;
    } else {
      [self resetGestureHandlerAndSimultaneous:node];
    }

    int state = [self getCurrentMemberState:node];

    if (state <= LYNX_STATE_ACTIVE) {
      return node;
    } else if (state == LYNX_STATE_END) {
      return nil;
    }
  }

  return nil;
}

/**
 Check if the current member is active based on the delta values.
 @param node    The GestureArenaMember to check.
 @return int state  True if the member is active, false otherwise.
 */
- (int)getCurrentMemberState:(id<LynxGestureArenaMember>)node {
  if (!node) {
    return LYNX_STATE_FAIL;
  }

  NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers = [node getGestureHandlers];
  if (!gestureHandlers) {
    return LYNX_STATE_FAIL;
  }

  int minStatus = -1;

  for (LynxBaseGestureHandler *handler in [gestureHandlers allValues]) {
    if ([handler isEnd]) {
      [self resetGestureHandlerAndSimultaneous:node];
      // if end api invoked, not re-compete gesture to last winner
      _lastWinner = nil;
      return LYNX_STATE_END;
    }

    if ([handler isActive]) {
      [self failOthersMembersInRaceRelation:node
                           currentGestureId:handler.gestureDetector.gestureID
                     simultaneousGestureIds:_simultaneousGestureIds];
      return LYNX_STATE_ACTIVE;
    }

    if (minStatus < 0) {
      minStatus = handler.status;
    } else if (minStatus > handler.status) {
      minStatus = handler.status;
    }
  }
  return minStatus;
}

/**
 * Dispatches touch event to the gesture handlers associated with the current winner in the
 * gesture arena.
 *

 */
- (void)dispatchUIEventOnCurrentWinner:(NSString *const)touchType
                               touches:(NSSet<UITouch *> *)touches
                                 event:(UIEvent *_Nullable)event
                                member:(id<LynxGestureArenaMember> _Nullable)member
                            touchEvent:(LynxTouchEvent *_Nullable)touchEvent
                            flingPoint:(CGPoint)flingPoint {
  // If there is no current member, return early as there are no gesture handlers to dispatch the
  // event to.
  if (!member) {
    return;
  }

  // Retrieve the gesture handlers associated with the current member.
  NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers = [member getGestureHandlers];
  // If there are no gesture handlers, return as there is no need to dispatch the event.
  if (!gestureHandlers) {
    return;
  }

  for (LynxBaseGestureHandler *handler in [gestureHandlers allValues]) {
    [handler handleUIEvent:touchType
                   touches:touches
                     event:event
                touchEvent:touchEvent
                flingPoint:flingPoint];
  }
}

// find next winner in onBegin callback when developer fail in continuous onBegin callback
- (void)findNextWinnerInBegin:(NSString *const)touchType
                      touches:(NSSet<UITouch *> *_Nullable)touches
                        event:(UIEvent *_Nullable)event
                   touchEvent:(LynxTouchEvent *)touchEvent
                        delta:(CGPoint)deltaPoint
       competeChainCandidates:(NSArray<id<LynxGestureArenaMember>> *)competeChainCandidates {
  for (NSUInteger idx = 0; idx < competeChainCandidates.count; idx++) {
    // limit the maximum number of the chain loops to prevent infinite loops
    if (self.winner == self.lastWinner || self.winner == nil) {
      return;
    }

    _lastWinner = self.winner;
    [self updateSimultaneousWinner:self.winner];

    [self dispatchUIEventWithSimultaneousAndReCompete:touchType
                                              touches:touches
                                                event:event
                                           touchEvent:touchEvent
                                               winner:_winner
                                                delta:deltaPoint
                               competeChainCandidates:competeChainCandidates];
  }
  // if all compete candidates not active, need to end this gesture
  [self dispatchUIEventWithSimultaneousAndReCompete:touchType
                                            touches:touches
                                              event:event
                                         touchEvent:touchEvent
                                             winner:self.lastWinner
                                              delta:deltaPoint
                             competeChainCandidates:competeChainCandidates];
}

- (void)dispatchUIEventWithSimultaneous:(NSString *const)actionType
                                touches:(NSSet<UITouch *> *_Nullable)touches
                                  event:(UIEvent *_Nullable)event
                             touchEvent:(LynxTouchEvent *)touchEvent
                                 winner:(id<LynxGestureArenaMember>)winner
                                  delta:(CGPoint)delta {
  [self dispatchUIEventWithSimultaneousAndReCompete:actionType
                                            touches:touches
                                              event:event
                                         touchEvent:touchEvent
                                             winner:winner
                                              delta:delta
                             competeChainCandidates:nil];
}

/**
  Dispatches the active event to gesture handlers based on the event type, gesture type mask, and
  delta values with simultaneous, re-compete gesture to current winner
 */
- (void)dispatchUIEventWithSimultaneousAndReCompete:(NSString *const)actionType
                                            touches:(NSSet<UITouch *> *_Nullable)touches
                                              event:(UIEvent *_Nullable)event
                                         touchEvent:(LynxTouchEvent *)touchEvent
                                             winner:(id<LynxGestureArenaMember>)winner
                                              delta:(CGPoint)delta
                             competeChainCandidates:(NSArray<id<LynxGestureArenaMember>> *_Nullable)
                                                        competeChainCandidates {
  if (!winner) {
    return;
  }
  [self dispatchUIEventOnCurrentWinner:actionType
                               touches:touches
                                 event:event
                                member:winner
                            touchEvent:touchEvent
                            flingPoint:delta];

  if (self.simultaneousWinners) {
    for (id<LynxGestureArenaMember> member in self.simultaneousWinners) {
      [self dispatchUIEventOnCurrentWinner:actionType
                                   touches:touches
                                     event:event
                                    member:member
                                touchEvent:touchEvent
                                flingPoint:delta];
    }
  }

  if (competeChainCandidates) {
    _winner = [self reCompeteByGestures:competeChainCandidates current:_winner];
  }
}

- (void)failOthersMembersInRaceRelation:(id<LynxGestureArenaMember> _Nullable)member
                       currentGestureId:(NSInteger)currentGestureId
                 simultaneousGestureIds:(NSSet<NSNumber *> *)simultaneousGestureIds {
  if (!member) {
    return;
  }

  // Retrieve the dictionary of gesture handlers associated with the member.
  NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers = [member getGestureHandlers];

  // If there are no gesture handlers for the member, there is no need to reset anything, so return.
  if (!gestureHandlers) {
    return;
  }

  // Iterate through each gesture handler associated with the member and fail them excluding
  // simultaneous members.
  for (LynxBaseGestureHandler *handler in [gestureHandlers allValues]) {
    if (handler.gestureDetector.gestureID != currentGestureId &&
        ![simultaneousGestureIds containsObject:@(handler.gestureDetector.gestureID)]) {
      [handler fail];
    }
  }
}

- (void)resetCandidatesGestures:
    (NSArray<id<LynxGestureArenaMember>> *_Nullable)competeChainCandidates {
  if (!competeChainCandidates) {
    return;
  }
  for (id<LynxGestureArenaMember> member in competeChainCandidates) {
    [self resetGestureHandlerAndSimultaneous:(member)];
  }
  _duplicatedMember = nil;
}

- (void)resetGestureHandlerAndSimultaneous:(id<LynxGestureArenaMember> _Nullable)member {
  [self resetGestureHandler:member];
  if (_simultaneousWinners) {
    for (id<LynxGestureArenaMember> member in _simultaneousWinners) {
      [self resetGestureHandler:member];
    }
  }
}

- (void)resetGestureHandler:(id<LynxGestureArenaMember>)member {
  // If the member is nil, there are no gesture handlers to reset, so return early.
  if (!member) {
    return;
  }

  // Retrieve the dictionary of gesture handlers associated with the member.
  NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers = [member getGestureHandlers];

  // If there are no gesture handlers for the member, there is no need to reset anything, so return.
  if (!gestureHandlers) {
    return;
  }

  // Iterate through each gesture handler associated with the member and reset them.
  for (LynxBaseGestureHandler *handler in [gestureHandlers allValues]) {
    [handler reset];
  }
}

- (void)stopFlingByLastFlingMember:(NSString *const)touchType
                  bubbleCandidates:(NSArray<id<LynxGestureArenaMember>> *)bubbleCandidates
            competeChainCandidates:
                (NSArray<id<LynxGestureArenaMember>> *_Nullable)competeChainCandidates
                           touches:(NSSet<UITouch *> *_Nullable)touches
                             event:(UIEvent *_Nullable)event
                        touchEvent:(LynxTouchEvent *)touchEvent {
  if (!bubbleCandidates) {
    return;
  }

  for (id<LynxGestureArenaMember> candidate in bubbleCandidates) {
    if ((_winner && _lastFlingTargetId == [candidate getGestureArenaMemberId]) ||
        _lastFlingTargetId == 0) {
      _lastFlingTargetId = 0;
      if (![_flinger isFinished]) {
        [_flinger stop];
        [self dispatchUIEventWithSimultaneousAndReCompete:touchType
                                                  touches:touches
                                                    event:event
                                               touchEvent:touchEvent
                                                   winner:_winner
                                                    delta:CGPointZero
                                   competeChainCandidates:competeChainCandidates];
        // when stop fling, not trigger tap event
        NSInteger lastIndex = [self getLastNotNullIndexOfNSPointer:_eventHandlers];
        if (lastIndex >= 0) {
          _eventHandler = [_eventHandlers pointerAtIndex:lastIndex];
          [_eventHandler onGestureRecognizedByEventTarget:candidate];
        }
        break;
      }
    }
  }
}

- (void)handleFling:(LynxGestureFlingTrigger *)sender {
  CGPoint point = sender.distance;
  CGPoint delta = CGPointMake(sender.lastDistance.x - point.x, sender.lastDistance.y - point.y);

  _lastFlingWinner = [self reCompeteByGestures:[_gestureArenaManager getCompetitionChainCandidates]
                                       current:_lastFlingWinner];

  [self findNextWinnerInBegin:nil
                      touches:nil
                        event:nil
                   touchEvent:nil
                        delta:CGPointZero
       competeChainCandidates:[_gestureArenaManager getCompetitionChainCandidates]];

  if (_lastFlingWinner) {
    switch (sender.state) {
      case LynxGestureFlingTriggerStateStart:
      case LynxGestureFlingTriggerStateUpdate: {
        _lastFlingTargetId = [_lastFlingWinner getGestureArenaMemberId];
        [self dispatchUIEventWithSimultaneous:nil
                                      touches:nil
                                        event:nil
                                   touchEvent:nil
                                       winner:_lastFlingWinner
                                        delta:delta];
      } break;
      case LynxGestureFlingTriggerStateEnd: {
        [self dispatchUIEventWithSimultaneousAndReCompete:nil
                                                  touches:nil
                                                    event:nil
                                               touchEvent:nil
                                                   winner:_lastFlingWinner
                                                    delta:CGPointMake(FLT_EPSILON, FLT_EPSILON)
                                   competeChainCandidates:[_gestureArenaManager
                                                              getCompetitionChainCandidates]];
      } break;
      default:
        break;
    }
  } else {
    _lastFlingTargetId = 0;
    [self.flinger stop];
  }
}

/**
 Handle the gesture detector state changes and performs actions accordingly.
 @param member     The GestureArenaMember associated with the state change.
 @param gestureId  The gesture ID of the gesture detector.
 @param state      The state of the gesture detector.
 */
- (void)handleGestureDetectorState:(id<LynxGestureArenaMember>)member
                         gestureId:(NSInteger)gestureId
                             state:(LynxGestureState)state {
  if (!member) {
    return;
  }
  LynxBaseGestureHandler *handler = [self getGestureHandlerWithMember:member gestureId:gestureId];
  switch (state) {
    case LynxGestureStateActive:
      break;
    case LynxGestureStateFail: {
      [handler fail];
    } break;
    case LynxGestureStateEnd:
      [handler end];
      break;
  }
}

/**
 * Retrieve the gesture handler by gesture ID associated with the GestureArenaMember.
 * @param member     The GestureArenaMember associated with the gesture handler.
 * @param gestureId  The gesture ID of the gesture handler.
 * @return           The BaseGestureHandler associated with the gesture ID, or null if not found.
 */
- (LynxBaseGestureHandler *)getGestureHandlerWithMember:(id<LynxGestureArenaMember>)member
                                              gestureId:(NSInteger)gestureId {
  __block LynxBaseGestureHandler *target = nil;
  [[member getGestureHandlers]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxBaseGestureHandler *_Nonnull handler, BOOL *_Nonnull stop) {
        if ([handler.gestureDetector gestureID] == gestureId) {
          target = handler;
          *stop = YES;
        }
      }];
  return target;
}

- (void)dealloc {
  _gestureArenaManager = nil;
  _gestureDetectorManager = nil;
  _simultaneousWinners = nil;
  _simultaneousGestureIds = nil;
}

@end
