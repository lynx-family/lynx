// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "AppDelegate.h"

@interface AppDelegate (DebugRouter)

- (void)registerDebugRouterMessageHandlers;
- (NSMutableDictionary<NSString *, id> *)debugRouterOpenPageResponseForURL:(NSString *)url;
- (NSMutableDictionary<NSString *, id> *)debugRouterClosePageResponse;

@end
