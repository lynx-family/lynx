// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollView.h"
#import "LynxUIScroller.h"

@implementation LynxScrollView

- (BOOL)gestureRecognizer:(UIPanGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UISwipeGestureRecognizer *)otherGestureRecognizer {
  [self disableGesturesRecursivelyIfNecessary:self.gestureConsumer];

  if (nil != self.parentScrollView &&
      [otherGestureRecognizer.view isKindOfClass:[UIScrollView class]] && self.enableNested) {
    return YES;
  }

  if ([otherGestureRecognizer.view isKindOfClass:NSClassFromString(@"UILayoutContainerView")]) {
    if ((otherGestureRecognizer.state == UIGestureRecognizerStateBegan ||
         otherGestureRecognizer.state == UIGestureRecognizerStatePossible)) {
      if (!self.scrollY && !self.bounces) {
        if (!self.isRTL && self.contentOffset.x <= 0) {
          return YES;
        }
        if (self.scrollY &&
            self.contentOffset.x >= self.contentSize.width - self.frame.size.width) {
          return YES;
        }
      }
    }
  }

  return NO;
}

- (void)setContentOffset:(CGPoint)contentOffset {
  if (_increaseFrequencyWithGesture && _gestureEnabled && !_duringGestureScroll &&
      (self.dragging || self.decelerating)) {
    // Ignore the scroll during pan and fling
    return;
  }

  [super setContentOffset:contentOffset];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:
        (nonnull UIGestureRecognizer *)otherGestureRecognizer {
  if (_forceCanScroll && [otherGestureRecognizer.view isKindOfClass:_blockGestureClass] &&
      otherGestureRecognizer.view.tag == _recognizedViewTag) {
    return YES;
  }
  return NO;
}

- (void)updateContentSize {
  if (_weakUIScroller) {
    [_weakUIScroller updateContentSize];
  }
}

@end  // LynxScrollView
