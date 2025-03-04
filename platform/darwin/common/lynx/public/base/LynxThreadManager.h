// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//  encapsulate thread operation with GCD

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXTHREADMANAGER_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXTHREADMANAGER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxThreadManager : NSObject

+ (void)createIOSThread:(NSString*)name runnable:(dispatch_block_t)runnable;
+ (BOOL)isMainQueue;
+ (void)runBlockInMainQueue:(dispatch_block_t _Nonnull)runnable;
+ (void)runInTargetQueue:(dispatch_queue_t)queue runnable:(dispatch_block_t)runnable;
+ (dispatch_queue_t)getCachedQueueWithPrefix:(NSString* _Nonnull)identifier;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXTHREADMANAGER_H_
