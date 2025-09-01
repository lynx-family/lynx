// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Horizontal.h>

@implementation LynxBaseScrollView (Horizontal)

- (CGFloat)getScrollOffsetHorizontally {
  return self.contentOffset.x;
}

- (void)setScrollContentSizeHorizontally:(CGFloat)contentSize {
  [self setContentSize:CGSizeMake(contentSize, self.frame.size.height)];
}

- (void)scrollByUnlimitedHorizontally:(CGFloat)delta {
  [self scrollToUnlimitedHorizontally:self.contentOffset.x + delta];
}

- (void)scrollByHorizontally:(CGFloat)delta {
  [self scrollToHorizontally:self.contentOffset.x + delta];
}

- (void)scrollToUnlimitedHorizontally:(CGFloat)offset {
  [self setContentOffset:CGPointMake(offset, self.contentOffset.y)];
}

- (void)scrollToHorizontally:(CGFloat)offset {
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeHorizontally:&scrollRange];
  offset = MIN(MAX(offset, scrollRange[0]), scrollRange[1]);
  [self scrollToUnlimitedHorizontally:offset];
}

- (void)animatedScrollToHorizontally:(CGFloat)offset
                            complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeHorizontally:&scrollRange];
  offset = MIN(MAX(offset, scrollRange[0]), scrollRange[1]);
  [self animatedScrollToUnlimitedHorizontally:offset complete:complete];
}

- (void)animatedScrollToUnlimitedHorizontally:(CGFloat)offset
                                     complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  [self setContentOffset:CGPointMake(offset, self.contentOffset.y) animated:YES];
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateAnimating];
  [self updateProgrammaticallyScrollFinishedCallback:complete];
}

- (void)getScrollRangeHorizontally:(CGFloat (*)[2])range {
  (*range)[0] = MIN(0, -self.contentInset.left);
  (*range)[1] = MAX(0, self.contentSize.width - self.frame.size.width + self.contentInset.right);
}

- (BOOL)canScrollForwardsHorizontally {
  CGFloat offset = [self getScrollOffsetHorizontally];
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeHorizontally:&scrollRange];
  if (offset >= scrollRange[1] - 1.0 / UIScreen.mainScreen.scale) {
    return NO;
  } else {
    return YES;
  }
}

- (BOOL)canScrollBackwardsHorizontally {
  CGFloat offset = [self getScrollOffsetHorizontally];
  CGFloat scrollRange[2] = {0, 0};
  if (offset <= scrollRange[0]) {
    return NO;
  } else {
    return YES;
  }
}

@end
