// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@interface LynxGlobalObserver : NSObject

- (void)notifyAnimationStart;
- (void)notifyAnimationEnd;
- (void)notifyLayout:(NSDictionary*)options;
- (void)notifyScroll:(NSDictionary*)options;
- (void)notifyProperty:(NSDictionary*)options;

@end
