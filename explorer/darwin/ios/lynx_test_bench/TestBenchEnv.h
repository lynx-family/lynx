// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchEnv : NSObject
+ (instancetype)sharedInstance;
@property NSString* testBenchUrlPrefix;
@property NSString* testBenchUrlSchema;
@property NSString* testBenchUrlHost;
@end

NS_ASSUME_NONNULL_END
