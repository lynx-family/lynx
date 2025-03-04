// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXLIFECYCLEDISPATCHER_H_
#define DARWIN_COMMON_LYNX_LYNXLIFECYCLEDISPATCHER_H_

#import <Foundation/Foundation.h>
#import "LynxViewClient.h"
#import "LynxViewClientV2.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxView;

@interface LynxLifecycleDispatcher : NSObject <LynxViewLifecycle, LynxViewLifecycleV2>

@property(nonatomic, readonly) NSArray<id<LynxViewBaseLifecycle>>* lifecycleClients;
@property(nonatomic, readwrite) int32_t instanceId;

- (void)addLifecycleClient:(id<LynxViewBaseLifecycle>)lifecycleClient;
- (void)removeLifecycleClient:(id<LynxViewBaseLifecycle>)lifecycleClient;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXLIFECYCLEDISPATCHER_H_
