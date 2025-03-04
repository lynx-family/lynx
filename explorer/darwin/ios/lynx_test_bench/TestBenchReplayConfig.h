// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchReplayConfig : NSObject
- (id)initWithProductUrl:(NSString*)url;
// The url of record product.
@property(nonatomic, readonly) NSString* url;
// use sourceUrl replace template.js.
@property(nonatomic, readonly) NSString* sourceUrl;
// whether replay gesture, false by default.
@property(nonatomic, readonly) BOOL replayGesture;
// whether restrict the height of lynxView, when size overflow screen. false by default.
@property(nonatomic, readonly) BOOL heightLimit;
@property(nonatomic, readonly) BOOL enablePreDecode;
@property(nonatomic, readonly) NSInteger thread_mode;
// To ensure the stability of the e2e test, a delay is provided, in milliseconds.
@property(nonatomic, readonly) NSInteger delayEndInterval;
// Only actions in this list can be replay.
@property(nonatomic, readonly) NSSet* canMockFuncName;
// Store all actions that need to be replayed after the page is reloaded.
@property(nonatomic, readonly) NSSet* reloadFuncName;

@property(nonatomic, readonly) BOOL enableAirStrictMode;

// Record file and source file requestCachePolicy, default is
// NSURLRequestReloadIgnoringLocalCacheData
@property(nonatomic) NSURLRequestCachePolicy requestCachePolicy;
@end

NS_ASSUME_NONNULL_END
