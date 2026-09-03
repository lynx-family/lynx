// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxDynamicComponentFetcher.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxResourceProvider.h>
#import <Lynx/LynxTemplateResourceFetcher.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxRecorderDynamicComponentFetcher : NSObject <LynxResourceProvider,
                                                           LynxDynamicComponentFetcher,
                                                           LynxGenericResourceFetcher,
                                                           LynxTemplateResourceFetcher>
@property NSDictionary* urlRedirectMap;

- (void)parse:(NSArray*)actionList;

@end

NS_ASSUME_NONNULL_END
