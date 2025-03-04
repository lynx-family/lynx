// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxModule.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchReplayDataModule : NSObject <LynxModule>
+ (void)addFunctionCallArray:(NSArray *)responseData;
+ (void)addCallbackDictionary:(NSDictionary *)callbackDictionary;
+ (void)setJSbIgnoredInfo:(NSArray *)info;
+ (void)setJsbSettings:(NSDictionary *)settings;
+ (NSString *)replayTimeEnvJScript;
+ (void)setTime:(int64_t)time;
@end

NS_ASSUME_NONNULL_END
