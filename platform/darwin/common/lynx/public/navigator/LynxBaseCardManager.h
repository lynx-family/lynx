// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_LYNXBASECARDMANAGER_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_LYNXBASECARDMANAGER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxHolder;
@class LynxView;
@class LynxRoute;
@class LynxLruCache;

@interface LynxBaseCardManager : NSObject

- (instancetype)init:(id<LynxHolder>)lynxHolder WithCapacity:(NSInteger)capacity;

/**
 * Binding LynxView
 */
- (void)registerInitLynxView:(LynxView*)LynxView;

/**
 * Push new card to stack
 * @param templateUrl template url of new card
 * @param param init param
 */
- (void)push:(NSString*)templateUrl param:(NSDictionary*)param;

/**
 * Replace current card
 * @param templateUrl template url of new card
 * @param param init param
 */
- (void)replace:(NSString*)templateUrl param:(NSDictionary*)param;

/**
 * Pop card from the top of stack
 */
- (void)pop;

/**
 * Notify navigate back
 */
- (BOOL)onNavigateBack;

/**
 * Notify app did enter foreground
 */
- (void)didEnterForeground;

/**
 * Notify app did enter background
 */
- (void)didEnterBackground;

- (void)clearCaches;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_LYNXBASECARDMANAGER_H_
