// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView+Horizontal.h>
#import <Lynx/LynxBaseScrollView+Vertical.h>

@implementation LynxBaseScrollView (Auto)

- (CGPoint)getScrollOffset {
  return self.contentOffset;
}

- (void)setScrollContentSize:(CGSize)contentSize {
  if (self.vertical) {
    [self setScrollContentSizeVertically:contentSize.height];
  } else {
    [self setScrollContentSizeHorizontally:contentSize.width];
  }
}

- (void)scrollByUnlimited:(CGPoint)delta {
  if (self.vertical) {
    [self scrollByUnlimitedVertically:delta.y];
  } else {
    [self scrollByUnlimitedHorizontally:delta.x];
  }
}

- (void)scrollBy:(CGPoint)delta {
  if (self.vertical) {
    [self scrollByVertically:delta.y];
  } else {
    [self scrollByHorizontally:delta.x];
  }
}

- (void)scrollToUnlimited:(CGPoint)offset {
  if (self.vertical) {
    [self scrollToUnlimitedVertically:offset.y];
  } else {
    [self scrollToUnlimitedHorizontally:offset.x];
  }
}

- (void)scrollTo:(CGPoint)offset {
  if (self.vertical) {
    [self scrollToVertically:offset.y];
  } else {
    [self scrollToHorizontally:offset.x];
  }
}

- (void)animatedScrollTo:(CGPoint)offset
                complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  if (self.vertical) {
    [self animatedScrollToVertically:offset.y complete:complete];
  } else {
    [self animatedScrollToHorizontally:offset.x complete:complete];
  }
}

- (void)animatedScrollToUnlimited:(CGPoint)offset
                         complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  if (self.vertical) {
    [self animatedScrollToUnlimitedVertically:offset.y complete:complete];
  } else {
    [self animatedScrollToUnlimitedHorizontally:offset.x complete:complete];
  }
}

- (void)getScrollRange:(CGFloat (*)[4])range {
  CGFloat scrollRangeHorizontal[2] = {0, 0};
  [self getScrollRangeHorizontally:&scrollRangeHorizontal];
  CGFloat scrollRangeVertical[2] = {0, 0};
  [self getScrollRangeVertically:&scrollRangeVertical];
  (*range)[0] = scrollRangeHorizontal[0];
  (*range)[1] = scrollRangeHorizontal[1];
  (*range)[2] = scrollRangeVertical[0];
  (*range)[3] = scrollRangeVertical[1];
}

- (BOOL)canScrollForwards {
  return self.vertical ? [self canScrollForwardsVertically] : [self canScrollForwardsHorizontally];
}

- (BOOL)canScrollBackwards {
  return self.vertical ? [self canScrollBackwardsVertically]
                       : [self canScrollBackwardsHorizontally];
}

@end
