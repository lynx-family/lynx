// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class TestBenchActionCallback;
@interface TestBenchPageManager : NSObject
+ (instancetype)sharedInstance;
- (void)registerTestBenchActionCallback:(TestBenchActionCallback *)callback;
- (void)replayPageFromOpenSchema:(NSDictionary *)params;
- (void)startReplay:(NSString *)url;
- (void)removeCurrTestBenchVC:(NSString *)pageName hasBeenPop:(BOOL)hasBeenPop;
@end

NS_ASSUME_NONNULL_END
