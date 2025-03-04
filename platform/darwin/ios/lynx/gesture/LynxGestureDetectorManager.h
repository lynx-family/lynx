// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxGestureArenaMember;
@class LynxGestureDetectorDarwin;
@class LynxGestureArenaManager;

/**
 * The GestureDetectorManager class manages the association between gesture detectors and arena
 * members. It provides methods to register and unregister gesture detectors for specific members,
 * convert response chains to competitor chains based on gesture relationships, handle simultaneous
 * winners, and perform other related operations.
 *
 * The class utilizes a map to store the mapping between gesture IDs and sets of associated member
 * IDs. It supports the registration and un-registration of gesture detectors, allowing multiple
 * gesture detectors to be associated with the same member. It also provides functionality to
 * convert a response chain to a competitor chain, considering gesture relationships such as waiting
 * for other gestures before execution.
 *
 * The class assumes that the associated GestureArenaMember objects and GestureDetector objects are
 * properly managed and provided externally. It does not handle the actual logic of gesture
 * detection or arena management.
 *
 */

@interface LynxGestureDetectorManager : NSObject

- (instancetype)initWithArenaManager:(LynxGestureArenaManager *)manager;

/**
 Convert the response chain to a competitor chain based on gesture relationships.
 @param responseList The response chain to convert.
 @return The converted compete chain.
 */
- (NSMutableArray<id<LynxGestureArenaMember>> *)convertResponseChainToCompeteChain:
    (NSArray<id<LynxGestureArenaMember>> *)responseList;

/**
 Handle simultaneous winners and returns a list of affected members.
 @param current The current GestureArenaMember.
 @return The list of simultaneous winners.
 */
- (nullable NSDictionary<NSString *, NSSet *> *)handleSimultaneousWinner:
    (id<LynxGestureArenaMember>)current;

/**
 Register a gesture detector for a member.
 @param memberId The ID of the member.
 @param gestureDetector The gesture detector to unregister.
 */
- (void)registerGestureDetector:(NSInteger)memberId
                       detector:(LynxGestureDetectorDarwin *)gestureDetector;

/**
 Unregister a gesture detector for a member.
 @param memberId The ID of the member.
 @param gestureDetector The gesture detector to unregister.
 */
- (void)unregisterGestureDetector:(NSInteger)memberId
                         detector:(LynxGestureDetectorDarwin *)gestureDetector;

@end

NS_ASSUME_NONNULL_END
