// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Public.h>
#import <Lynx/UIScrollView+Lynx.h>

@implementation LynxBaseScrollView (Public)

- (void)enableScroll:(BOOL)enable {
  self.scrollEnabled = enable;
}

- (BOOL)scrollEnabled {
  return self.isScrollEnabled;
}

- (void)stopScrolling {
  [self setContentOffset:self.contentOffset animated:NO];
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
  [self stopScroll];
}

- (LynxBaseScrollViewScrollState)currentScrollState {
  return self.scrollState;
}

@end
