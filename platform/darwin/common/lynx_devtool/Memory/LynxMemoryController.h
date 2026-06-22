// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxMemoryListener.h>

typedef void (^LynxMemoryUsageResultCallback)(NSString* resultJson, NSString* errorMessage);

@interface LynxMemoryController : NSObject <LynxMemoryReporter>

+ (instancetype)shareInstance;

- (void)uploadImageInfo:(NSDictionary*)data;

- (void)startMemoryTracing;

- (void)stopMemoryTracing;

- (void)queryAllMemoryUsageWithTimeoutMs:(int64_t)timeoutMs
                                callback:(LynxMemoryUsageResultCallback)callback;

@end
