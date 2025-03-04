// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGestureDetectorManager.h"
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import "LynxGestureArenaManager.h"

static NSString *const LYNX_GESTURE_DETECTOR_WAIT_FOR = @"waitFor";

static NSString *const LYNX_GESTURE_DETECTOR_SIMULTANEOUS = @"simultaneous";

static NSString *const LYNX_GESTURE_DETECTOR_CONTINUE_WITH = @"continueWith";

@interface LynxGestureDetectorManager ()

@property(nonatomic, strong)
    NSMutableDictionary<NSNumber *, NSMutableSet<NSNumber *> *> *gestureToArenaMembers;

@property(nonatomic, weak) LynxGestureArenaManager *arenaManager;
@end

@implementation LynxGestureDetectorManager

- (instancetype)initWithArenaManager:(LynxGestureArenaManager *)manager {
  if (self = [super init]) {
    _gestureToArenaMembers = [NSMutableDictionary dictionary];
    _arenaManager = manager;
  }
  return self;
}

- (NSMutableArray<id<LynxGestureArenaMember>> *)convertResponseChainToCompeteChain:
    (NSArray<id<LynxGestureArenaMember>> *)responseList {
  NSMutableArray<id<LynxGestureArenaMember>> *result = [NSMutableArray array];
  if (!responseList || responseList.count == 0) {
    return result;
  }

  for (__block NSUInteger i = 0; i < responseList.count; i++) {
    id<LynxGestureArenaMember> node = responseList[i];
    NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *map = [node getGestureDetectorMap];

    if (!map) {
      // keep the original response chain relationship
      [result addObject:node];
      continue;
    }

    __block NSArray<NSNumber *> *waitForList = nil;
    __block NSArray<NSNumber *> *continueWithList = nil;

    NSArray<NSNumber *> *sortedKeys = [[map allKeys] sortedArrayUsingSelector:@selector(compare:)];

    for (NSNumber *key in sortedKeys) {
      // TODO(luochangan.adrian): Gesture relations need distinguish between types
      LynxGestureDetectorDarwin *obj = map[key];
      NSDictionary *relationMap = [obj relationMap];
      waitForList = relationMap[LYNX_GESTURE_DETECTOR_WAIT_FOR];
      continueWithList = relationMap[LYNX_GESTURE_DETECTOR_CONTINUE_WITH];
      if ((waitForList != nil && [waitForList count] > 0) ||
          (continueWithList != nil && [continueWithList count] > 0)) {
        // when waitForList or continueWithList not empty, exit loop
        break;
      }
    }

    if ((!continueWithList || continueWithList.count == 0) &&
        (!waitForList || waitForList.count == 0)) {
      // keep the original response chain relationship
      [result addObject:node];
      continue;
    }

    if (waitForList && waitForList.count > 0) {
      // handle waitFor relation
      NSMutableOrderedSet<NSNumber *> *arenaMembers = [NSMutableOrderedSet orderedSet];
      for (NSNumber *index in waitForList) {
        if (self.gestureToArenaMembers[index]) {
          [arenaMembers unionSet:self.gestureToArenaMembers[index]];
        }
      }

      NSArray<NSNumber *> *indexList = [self findCandidatesAfterCurrentInChain:responseList
                                                                       current:node
                                                                       members:arenaMembers];
      if (!indexList || indexList.count == 0) {
        [result addObject:node];
        continue;
      }
      for (NSNumber *index in indexList) {
        if (responseList[index.integerValue]) {
          [result addObject:responseList[index.integerValue]];
        }
      }

      [result addObject:node];
      // skip to the last GestureArenaMember
      i = responseList.count;
    } else {
      [result addObject:node];
    }

    if (continueWithList && continueWithList.count > 0) {
      // handle continueWith relation
      NSMutableOrderedSet<NSNumber *> *arenaMembers = [NSMutableOrderedSet orderedSet];
      for (NSNumber *index in continueWithList) {
        if (self.gestureToArenaMembers[index]) {
          [arenaMembers unionSet:self.gestureToArenaMembers[index]];
        }
      }

      NSArray<id<LynxGestureArenaMember>> *continueWithMembers =
          [self findCandidatesFromArenaMember:node
                        gestureToArenaMembers:self.gestureToArenaMembers
                                 arenaMembers:arenaMembers];

      if (continueWithMembers && continueWithMembers.count > 0) {
        [result addObjectsFromArray:continueWithMembers];
      }
      break;
    }
  }

  return result;
}

- (NSArray<NSNumber *> *)
    findCandidatesAfterCurrentInChain:(NSArray<id<LynxGestureArenaMember>> *)responseList
                              current:(id<LynxGestureArenaMember>)current
                              members:(NSMutableOrderedSet<NSNumber *> *)arenaMembers {
  if (!responseList || !arenaMembers) {
    return nil;
  }

  NSUInteger index = [responseList indexOfObject:current];
  if (index == NSNotFound) {
    return nil;
  }

  NSMutableArray<NSNumber *> *indexList = [NSMutableArray array];

  // Given the response chain: A -> B -> C, if the following rules exist:
  // A waitFor C, A waitFor B, the expected final wait chain is C -> B -> A
  // A waitFor B, A waitFor C, the expected final wait chain is B -> C -> A
  for (NSNumber *memberId in arenaMembers) {
    for (NSUInteger i = index + 1; i < responseList.count; i++) {
      id<LynxGestureArenaMember> member = responseList[i];
      if ([member getGestureArenaMemberId] == memberId.intValue) {
        [indexList addObject:@(i)];
      }
    }
  }
  return indexList;
}

- (nullable NSDictionary<NSString *, NSSet *> *)handleSimultaneousWinner:
    (id<LynxGestureArenaMember>)current {
  if (!current || !self.arenaManager) {
    return nil;
  }

  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *map = [current getGestureDetectorMap];

  if (!map.count) {
    return nil;
  }

  NSMutableSet<id<LynxGestureArenaMember>> *results = [NSMutableSet set];
  NSMutableSet<NSNumber *> *currentGestureIds = [NSMutableSet set];
  NSMutableSet<NSNumber *> *simultaneousGestureIds = [NSMutableSet set];

  for (NSNumber *key in map.allKeys) {
    LynxGestureDetectorDarwin *obj = map[key];
    [currentGestureIds addObject:@([obj gestureID])];
  }

  for (NSNumber *key in map.allKeys) {
    LynxGestureDetectorDarwin *obj = map[key];
    NSArray<NSNumber *> *simultaneousList =
        [[obj relationMap] objectForKey:LYNX_GESTURE_DETECTOR_SIMULTANEOUS];

    for (NSNumber *i in simultaneousList) {
      if ([currentGestureIds containsObject:i]) {
        [simultaneousGestureIds addObject:i];
        continue;
      }

      NSSet<NSNumber *> *memberSets =
          [NSSet setWithArray:[[self gestureToArenaMembers][i] allObjects]];

      for (NSNumber *memberID in memberSets) {
        id<LynxGestureArenaMember> member =
            [self.arenaManager getMemberById:[memberID integerValue]];

        if (member && [member getGestureArenaMemberId] != [current getGestureArenaMemberId]) {
          [results addObject:member];
        }
      }
    }
  }

  return
      @{@"LynxGestureArenaMembers" : results, @"SimultaneousGestureIds" : simultaneousGestureIds};
}

- (NSArray<id<LynxGestureArenaMember>> *)
    findCandidatesFromArenaMember:(id<LynxGestureArenaMember>)current
            gestureToArenaMembers:
                (NSDictionary<NSNumber *, NSSet<NSNumber *> *> *)gestureToArenaMembers
                     arenaMembers:(NSMutableOrderedSet<NSNumber *> *)arenaMembers {
  NSMutableArray<id<LynxGestureArenaMember>> *resultList = [NSMutableArray array];

  if (!current || !gestureToArenaMembers || !arenaMembers) {
    return resultList;
  }

  [arenaMembers enumerateObjectsUsingBlock:^(NSNumber *obj, NSUInteger idx, BOOL *stop) {
    [resultList addObject:[self.arenaManager getMemberById:[obj integerValue]]];
  }];
  return resultList;
}

- (void)registerGestureDetector:(NSInteger)memberId
                       detector:(LynxGestureDetectorDarwin *)gestureDetector {
  [self registerGestureId:[gestureDetector gestureID] withMemberId:memberId];
}

- (void)unregisterGestureDetector:(NSInteger)memberId
                         detector:(LynxGestureDetectorDarwin *)gestureDetector {
  [self unregisterGestureId:[gestureDetector gestureID] withMemberId:memberId];
}

- (void)registerGestureId:(NSInteger)gestureId withMemberId:(NSInteger)memberId {
  NSMutableSet<NSNumber *> *set = [_gestureToArenaMembers objectForKey:@(gestureId)];
  if (!set) {
    set = [NSMutableSet set];
    [_gestureToArenaMembers setObject:set forKey:@(gestureId)];
  }
  [set addObject:@(memberId)];
}

- (void)unregisterGestureId:(NSInteger)gestureId withMemberId:(NSInteger)memberId {
  NSMutableSet<NSNumber *> *set = [_gestureToArenaMembers objectForKey:@(gestureId)];
  [set removeObject:@(memberId)];
  if (![set count]) {
    [_gestureToArenaMembers removeObjectForKey:@(gestureId)];
  }
}

- (void)dealloc {
  _gestureToArenaMembers = nil;
}

@end
