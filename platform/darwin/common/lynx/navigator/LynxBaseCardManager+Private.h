// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_NAVIGATOR_LYNXBASECARDMANAGER_PRIVATE_H_
#define DARWIN_COMMON_LYNX_NAVIGATOR_LYNXBASECARDMANAGER_PRIVATE_H_

#import "LynxBaseCardManager.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxBaseCardManager ()

/**
 * Related LynxView
 */
@property(nonatomic, strong) LynxView* lynxView;
/**
 * Card stack
 */
@property(nonatomic, strong) NSMutableArray* routeStack;
@property(nonatomic, strong) LynxLruCache* lruCache;
/**
 * Page Holder
 */
@property(nonatomic, weak) id<LynxHolder> holder;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_NAVIGATOR_LYNXBASECARDMANAGER_PRIVATE_H_
