// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxDynamicComponentFetcher.h>
#import <Lynx/LynxResourceProvider.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchDynamicComponentFetcher
    : NSObject <LynxResourceProvider, LynxDynamicComponentFetcher>
@property NSDictionary* urlRedirectMap;

- (void)parse:(NSArray*)actionList;

@end

NS_ASSUME_NONNULL_END
