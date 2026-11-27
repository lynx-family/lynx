// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxMapProvider.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxXElementMapService : NSObject
/// Register the map implementation before creating any map elements.
+ (void)registerProvider:(id<LynxMapProvider>)provider NS_SWIFT_NAME(registerProvider(_:));
+ (id<LynxMapProvider>)provider;
@end

NS_ASSUME_NONNULL_END
