// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxScrollView.h>
#import <Lynx/LynxUIScroller.h>

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

  if (gestureRecognizer == _nativeGesturePanRecognizer && _gestureConsumer &&
      _gestureConsumer.interceptGestureStatus == LynxInterceptGestureStateTrue) {
    otherGestureRecognizer.state = UIGestureRecognizerStateFailed;
    return NO;
  }

  return NO;
}

- (void)handlePanGesture:(UIPanGestureRecognizer *)recognizer {
  if (_gestureConsumer &&
      _gestureConsumer.interceptGestureStatus == LynxInterceptGestureStateTrue) {
    return;
  }
  recognizer.state = UIGestureRecognizerStateCancelled;
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
  if (gestureRecognizer == self.nativeGesturePanRecognizer) {
    return _gestureConsumer.interceptGestureStatus == LynxInterceptGestureStateTrue;
  }
  return YES;
}

- (void)setupNativeGestureRecognizerIfNeeded:
    (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureMap {
  for (NSNumber *key in gestureMap) {
    LynxGestureDetectorDarwin *detector = gestureMap[key];
    // Check if a native gesture type exists.
    if (detector.gestureType == LynxGestureTypeNative) {
      // If found, ensure the native pan recognizer is set up on the scrollview.
      if (!self.nativeGesturePanRecognizer) {
        self.nativeGesturePanRecognizer =
            [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                    action:@selector(handlePanGesture:)];
        self.nativeGesturePanRecognizer.delegate = self;
        [self addGestureRecognizer:self.nativeGesturePanRecognizer];
      }
      break;
    }
  }
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

- (void)dealloc {
  // Remove the gesture recognizer from the view
  if (_nativeGesturePanRecognizer) {
    _nativeGesturePanRecognizer.delegate = nil;
    [self removeGestureRecognizer:_nativeGesturePanRecognizer];
    _nativeGesturePanRecognizer = nil;
  }
}

@end  // LynxScrollView
