// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Vertical.h>

@implementation LynxBaseScrollView (Vertical)

- (CGFloat)getScrollOffsetVertically {
  return self.contentOffset.y;
}

- (void)setScrollContentSizeVertically:(CGFloat)contentSize {
  [self setContentSize:CGSizeMake(self.frame.size.width, contentSize)];
}

- (void)scrollByUnlimitedVertically:(CGFloat)delta {
  [self scrollToUnlimitedVertically:self.contentOffset.y + delta];
}

- (void)scrollByVertically:(CGFloat)delta {
  [self scrollToVertically:self.contentOffset.y + delta];
}

- (void)scrollToUnlimitedVertically:(CGFloat)offset {
  [self setContentOffset:CGPointMake(self.contentOffset.x, offset)];
}

- (void)scrollToVertically:(CGFloat)offset {
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeVertically:&scrollRange];
  offset = MIN(MAX(offset, scrollRange[0]), scrollRange[1]);
  [self scrollToUnlimitedVertically:offset];
}

- (void)animatedScrollToVertically:(CGFloat)offset
                          complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeVertically:&scrollRange];
  offset = MIN(MAX(offset, scrollRange[0]), scrollRange[1]);
  [self animatedScrollToUnlimitedVertically:offset complete:complete];
}

- (void)animatedScrollToUnlimitedVertically:(CGFloat)offset
                                   complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  [self setContentOffset:CGPointMake(self.contentOffset.x, offset) animated:YES];
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateAnimating];
  [self updateProgrammaticallyScrollFinishedCallback:complete];
}

- (void)getScrollRangeVertically:(CGFloat (*)[2])range {
  (*range)[0] = MIN(0, -self.contentInset.top);
  (*range)[1] = MAX(0, self.contentSize.height - self.frame.size.height + self.contentInset.bottom);
}

- (BOOL)canScrollForwardsVertically {
  CGFloat offset = [self getScrollOffsetVertically];
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeVertically:&scrollRange];
  if (offset >= scrollRange[1] - 1.0 / UIScreen.mainScreen.scale) {
    return NO;
  } else {
    return YES;
  }
}

- (BOOL)canScrollBackwardsVertically {
  CGFloat offset = [self getScrollOffsetVertically];
  CGFloat scrollRange[2] = {0, 0};
  [self getScrollRangeVertically:&scrollRange];
  if (offset <= scrollRange[0]) {
    return NO;
  } else {
    return YES;
  }
}

@end
