// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxUITarget <NSObject>
@optional
- (void)targetOnScreen;
- (void)freeMemoryCache;
- (void)targetOffScreen;
@end

NS_ASSUME_NONNULL_END
