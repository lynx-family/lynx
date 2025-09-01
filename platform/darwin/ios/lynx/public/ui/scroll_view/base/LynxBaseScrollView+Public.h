// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView+Horizontal.h>
#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/LynxBaseScrollView+Vertical.h>
#import <Lynx/LynxBaseScrollView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxBaseScrollView (Public)

- (void)enableScroll:(BOOL)enable;

- (BOOL)scrollEnabled;

- (void)stopScrolling;

- (LynxBaseScrollViewScrollState)currentScrollState;

@end

NS_ASSUME_NONNULL_END
