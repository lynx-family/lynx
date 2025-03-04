// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXDYNAMICCOMPONENTFETCHER_H_
#define DARWIN_COMMON_LYNX_LYNXDYNAMICCOMPONENTFETCHER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^onComponentLoaded)(NSData* _Nullable data, NSError* _Nullable error);

API_DEPRECATED_WITH_REPLACEMENT("LynxTemplateResourceFetcher", ios(6.0, API_TO_BE_DEPRECATED))
@protocol LynxDynamicComponentFetcher <NSObject>

- (void)loadDynamicComponent:(NSString*)url withLoadedBlock:(onComponentLoaded)block;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXDYNAMICCOMPONENTFETCHER_H_
