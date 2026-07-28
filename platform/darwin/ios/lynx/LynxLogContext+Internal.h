// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_LYNXLOGCONTEXT_INTERNAL_H_
#define PLATFORM_DARWIN_IOS_LYNX_LYNXLOGCONTEXT_INTERNAL_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxLogContext : NSObject

@property(nonatomic, readonly, assign) int64_t viewId;
@property(nonatomic, readonly, assign) int64_t engineId;
@property(nonatomic, readonly, assign) int64_t runtimeId;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (instancetype)initWithViewId:(int64_t)viewId
                      engineId:(int64_t)engineId
                     runtimeId:(int64_t)runtimeId;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_LYNXLOGCONTEXT_INTERNAL_H_
