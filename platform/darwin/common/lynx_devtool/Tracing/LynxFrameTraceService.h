// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@interface LynxFrameTraceService : NSObject
+ (instancetype)shareInstance;
- (void)initializeService;
- (void)FPSTrace:(const uint64_t)startTime withEndTime:(const uint64_t)endTime;
- (void)screenshot:(NSString *)snapshot;
@end
