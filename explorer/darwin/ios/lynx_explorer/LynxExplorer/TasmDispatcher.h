// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Deprecated compatibility facade. All logic is forwarded to the installed
/// RouteCoordinator (`LXRouteCoordinator`); new callers should route through it
/// directly. Kept only for older native / E2E callers during migration.
@interface TasmDispatcher : NSObject

+ (instancetype)sharedInstance;
/// Compatibility facade for older native callers. Routing and navigation are
/// owned by the installed RouteCoordinator.
- (void)openTargetUrl:(NSString*)sourceUrl
    __attribute__((deprecated("Use LXRouteCoordinator openModuleURL: routing.")));
/// Replaces the current top controller for existing E2E callers.
- (void)openTargetUrlSingleTop:(NSString*)sourceUrl
    __attribute__((deprecated("Use LXRouteCoordinator openModuleURL: routing.")));

@end

NS_ASSUME_NONNULL_END
