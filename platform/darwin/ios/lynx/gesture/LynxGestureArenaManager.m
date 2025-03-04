// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGestureArenaManager.h"
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxTouchEvent.h>
#import "LynxGestureDetectorManager.h"
#import "LynxGestureHandlerTrigger.h"

@interface LynxGestureArenaManager ()

@property(nonatomic, assign) NSInteger potentialMemberId;

@property(nonatomic, strong) id<LynxGestureArenaMember> winner;

@property(nonatomic, strong)
    NSMutableDictionary<NSNumber *, id<LynxGestureArenaMember>> *arenaMemberMap;

@property(nonatomic, strong) NSMutableArray<id<LynxGestureArenaMember>> *bubbleCandidate;

@property(nonatomic, strong) LynxGestureDetectorManager *gestureDetectorManager;

@property(nonatomic, strong) NSMutableArray<id<LynxGestureArenaMember>> *competitionChainCandidates;

@end

@implementation LynxGestureArenaManager

- (instancetype)init {
  if (self = [super init]) {
    _arenaMemberMap = [NSMutableDictionary dictionary];
    _bubbleCandidate = [NSMutableArray array];
    _gestureDetectorManager = [[LynxGestureDetectorManager alloc] initWithArenaManager:self];
    _gestureHandlerTrigger =
        [[LynxGestureHandlerTrigger alloc] initWithDetectorManager:_gestureDetectorManager
                                                      arenaManager:self];
  }
  return self;
}

- (NSArray<id<LynxGestureArenaMember>> *)getCompetitionChainCandidates {
  return [self.competitionChainCandidates copy];
}

- (void)dispatchTouchToArena:(NSString *const)touchType
                     touches:(NSSet<UITouch *> *)touches
                       event:(UIEvent *)event
                  touchEvent:(LynxTouchEvent *)touchEvent {
  [self.gestureHandlerTrigger resolveTouchEvent:touchType
                                        touches:touches
                                          event:event
                                     touchEvent:touchEvent
                      completionChainCandidates:self.competitionChainCandidates
                               bubbleCandidates:self.bubbleCandidate];
}

- (void)dispatchBubble:(NSString *const)touchType touchEvent:(LynxTouchEvent *)touchEvent {
  [self.gestureHandlerTrigger dispatchBubble:touchType
                                  touchEvent:touchEvent
                             bubbleCandidate:self.bubbleCandidate
                                      winner:self.winner];
}

- (void)setActiveUIToArena:(id<LynxEventTarget>)target {
  [self clearCurrentGesture];

  if (!self.arenaMemberMap.count) {
    return;
  }

  id<LynxEventTarget> temp = target;

  while (temp) {
    [self.arenaMemberMap enumerateKeysAndObjectsUsingBlock:^(
                             NSNumber *_Nonnull key, id<LynxGestureArenaMember> _Nonnull member,
                             BOOL *_Nonnull stop) {
      if ([member getGestureArenaMemberId] > 0 &&
          [member getGestureArenaMemberId] == [temp getGestureArenaMemberId]) {
        [self.bubbleCandidate addObject:member];
      }
    }];

    temp = [temp parentTarget];
  }

  self.competitionChainCandidates =
      [self.gestureDetectorManager convertResponseChainToCompeteChain:self.bubbleCandidate];

  self.winner = [self.competitionChainCandidates firstObject];

  [self.gestureHandlerTrigger setCurrentWinnerWhenDown:self.winner];
}

/**
 * Clears the current gesture state.
 */
- (void)clearCurrentGesture {
  self.winner = nil;

  [_bubbleCandidate removeAllObjects];
  _competitionChainCandidates = nil;
}

- (NSInteger)addMember:(id<LynxGestureArenaMember>)member {
  [self.arenaMemberMap setObject:member forKey:@(++self.potentialMemberId)];
  return self.potentialMemberId;
}

- (BOOL)isMemberExist:(NSInteger)memberId {
  return [self.arenaMemberMap objectForKey:@(memberId)];
}

- (id<LynxGestureArenaMember>)getMemberById:(NSInteger)memberId {
  return [self.arenaMemberMap objectForKey:@(memberId)];
}

- (void)removeMember:(id<LynxGestureArenaMember>)member
         detectorMap:(NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)detectorMap {
  [self.arenaMemberMap removeObjectForKey:@([member getGestureArenaMemberId])];
  [self unregisterGestureDetectors:[member getGestureArenaMemberId] detectorMap:detectorMap];
}

- (void)registerGestureDetectors:(NSInteger)memberId
                     detectorMap:
                         (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors {
  [gestureDetectors
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.gestureDetectorManager registerGestureDetector:memberId detector:obj];
      }];
}

- (void)unregisterGestureDetectors:(NSInteger)memberId
                       detectorMap:(NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)
                                       gestureDetectors {
  [gestureDetectors
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.gestureDetectorManager unregisterGestureDetector:memberId detector:obj];
      }];
}

- (void)setGestureDetectorState:(NSInteger)gestureId
                       memberId:(NSInteger)memberId
                          state:(LynxGestureState)state {
  if (_arenaMemberMap) {
    [_gestureHandlerTrigger handleGestureDetectorState:_arenaMemberMap[@(memberId)]
                                             gestureId:gestureId
                                                 state:state];
  }
}

- (void)dealloc {
  _arenaMemberMap = nil;
  _bubbleCandidate = nil;
  _gestureDetectorManager = nil;
  _gestureHandlerTrigger = nil;
  _competitionChainCandidates = nil;
}

@end
