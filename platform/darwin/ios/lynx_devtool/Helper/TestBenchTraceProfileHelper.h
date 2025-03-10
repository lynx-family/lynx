// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchTraceProfileHelper : NSObject

- (void)startTrace;
- (void)startTrace:(int)bufferSize;
- (void)stopTrace;

@end

NS_ASSUME_NONNULL_END
