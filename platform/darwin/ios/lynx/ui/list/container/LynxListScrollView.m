// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListScrollView.h"

#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/UIScrollView+Lynx.h>
#import <Lynx/UIScrollView+LynxGesture.h>
#import <Lynx/UIScrollView+Nested.h>
#import <objc/runtime.h>

// Declare BaseScrollView's existing private delegate methods for super forwarding.
@interface LynxBaseScrollView (LynxListDelegateForwarding)
- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset;
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer;
@end

@interface LynxListScrollView ()
@property(nonatomic, strong, nullable) UIPanGestureRecognizer *nativeGesturePanRecognizer;
@property(nonatomic, assign) NSUInteger autoScrollGeneration;
@end

@implementation LynxListScrollView

- (instancetype)init {
  return [self initWithVertical:YES layoutFromEnd:NO];
}

- (instancetype)initWithVertical:(BOOL)vertical layoutFromEnd:(BOOL)layoutFromEnd {
  self = [super initWithVertical:vertical layoutFromEnd:layoutFromEnd];
  if (self) {
    _verticalOrientation = vertical;
    _scrollEstimatedOffset = LynxListScrollInvalidEstimatedOffset;
    self.scrollY = vertical;
  }
  return self;
}

- (void)setUi:(id<LynxListScrollViewOwner> _Nullable)ui {
  _ui = ui;
  self.scrollDelegate = ui;
}

- (void)setVerticalOrientation:(BOOL)verticalOrientation {
  _verticalOrientation = verticalOrientation;
  self.vertical = verticalOrientation;
  self.scrollY = verticalOrientation;
}

- (void)willMoveToWindow:(UIWindow *)newWindow {
  [super willMoveToWindow:newWindow];
  if (!newWindow) {
    [self.ui detachedFromWindow];
  }
}

- (CGPoint)listConstrainedContentOffset:(CGPoint)contentOffset {
  return [LynxListScrollHelper contentOffset:contentOffset
                constrainedToEstimatedOffset:_scrollEstimatedOffset
                                    vertical:_verticalOrientation
                               scrollToLower:_scrollToLower];
}

- (BOOL)shouldIgnoreContentOffsetUpdate {
  // Keep list-container's gesture-frequency path as the sole offset writer while dragging.
  return _increaseFrequencyWithGesture && _gestureEnabled && !_duringGestureScroll &&
         (self.dragging || self.decelerating);
}

- (void)setContentOffset:(CGPoint)contentOffset {
  if ([self shouldIgnoreContentOffsetUpdate]) {
    return;
  }
  contentOffset = [self listConstrainedContentOffset:contentOffset];
  if (!CGPointEqualToPoint(self.contentOffset, contentOffset)) {
    [super setContentOffset:contentOffset];
  }
}

- (void)setContentOffset:(CGPoint)contentOffset animated:(BOOL)animated {
  if ([self shouldIgnoreContentOffsetUpdate]) {
    return;
  }
  contentOffset = [self listConstrainedContentOffset:contentOffset];
  if (animated && !CGPointEqualToPoint(self.contentOffset, contentOffset)) {
    [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateAnimating];
  }
  [super setContentOffset:contentOffset animated:animated];
}

- (void)setContentSize:(CGSize)contentSize {
  CGSize oldSize = self.contentSize;
  [super setContentSize:contentSize];
  CGSize newSize = self.contentSize;
  if (!CGSizeEqualToSize(oldSize, newSize)) {
    [self.ui listScrollView:self contentSizeChangedFrom:oldSize to:newSize];
  }
}

- (BOOL)scrollViewShouldScrollToTop:(UIScrollView *)scrollView {
  return [self.ui listScrollViewShouldScrollToTop:scrollView];
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView
                     withVelocity:(CGPoint)velocity
              targetContentOffset:(inout CGPoint *)targetContentOffset {
  [self.ui listScrollViewWillEndDragging:scrollView
                            withVelocity:velocity
                     targetContentOffset:targetContentOffset];
  [super scrollViewWillEndDragging:scrollView
                      withVelocity:velocity
               targetContentOffset:targetContentOffset];
}

- (void)autoScrollWithRate:(CGFloat)rate
                  behavior:(LynxScrollViewTouchBehavior)behavior
                  interval:(NSTimeInterval)interval
                  autoStop:(BOOL)autoStop
                  vertical:(BOOL)isVertical
                  complete:(UIScrollViewLynxCompletion _Nullable)callback {
  NSUInteger generation = ++self.autoScrollGeneration;
  __weak __typeof(self) weakSelf = self;
  [super autoScrollWithRate:rate
                   behavior:behavior
                   interval:interval
                   autoStop:autoStop
                   vertical:isVertical
                   complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
                     __strong __typeof(weakSelf) strongSelf = weakSelf;
                     if (strongSelf && generation == strongSelf.autoScrollGeneration) {
                       [strongSelf.ui listScrollViewAutoScrollDidStop];
                     }
                     // Keep list-container compatibility: the list state hook above owns the
                     // single scrollend event when unified auto-scroll reaches its boundary.
                     return callback ? callback(scrollEnabledAtStart, completed)
                                     : scrollEnabledAtStart;
                   }];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  [self disableGesturesRecursivelyIfNecessary:self.gestureConsumer];

  // Preserve list-container's legacy nested mode in addition to BaseScrollView's nested modes.
  if (nil != self.parentScrollView &&
      [otherGestureRecognizer.view isKindOfClass:[UIScrollView class]] && self.enableNested) {
    return YES;
  }

  // Preserve list-container's horizontal navigation gesture arbitration, including RTL.
  Class layoutContainerViewClass = objc_getClass("UILayoutContainerView");
  if (layoutContainerViewClass &&
      [otherGestureRecognizer.view isKindOfClass:layoutContainerViewClass]) {
    if ((otherGestureRecognizer.state == UIGestureRecognizerStateBegan ||
         otherGestureRecognizer.state == UIGestureRecognizerStatePossible) &&
        !self.scrollY && !self.bounces) {
      if (!self.isRTL && self.contentOffset.x <= 0) {
        return YES;
      }
      if (self.isRTL && self.contentOffset.x >= self.contentSize.width - self.frame.size.width) {
        return YES;
      }
    }
  }

  if (gestureRecognizer == _nativeGesturePanRecognizer && _gestureConsumer &&
      _gestureConsumer.interceptGestureStatus == LynxInterceptGestureStateTrue) {
    otherGestureRecognizer.state = UIGestureRecognizerStateFailed;
    return NO;
  }

  return [super gestureRecognizer:gestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:otherGestureRecognizer];
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

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:
        (nonnull UIGestureRecognizer *)otherGestureRecognizer {
  return _forceCanScroll && [otherGestureRecognizer.view isKindOfClass:_blockGestureClass] &&
         otherGestureRecognizer.view.tag == _recognizedViewTag;
}

- (void)setupNativeGestureRecognizerIfNeeded:
    (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureMap {
  for (NSNumber *key in gestureMap) {
    LynxGestureDetectorDarwin *detector = gestureMap[key];
    if (detector.gestureType == LynxGestureTypeNative) {
      if (!self.nativeGesturePanRecognizer) {
        self.nativeGesturePanRecognizer =
            [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                    action:@selector(handlePanGesture:)];
        self.nativeGesturePanRecognizer.delegate = self;
        [self addGestureRecognizer:self.nativeGesturePanRecognizer];
      }
      return;
    }
  }
  [self removeNativeGestureRecognizer];
}

- (void)updateContentSize {
  [self.ui updateContentSize];
}

- (void)removeNativeGestureRecognizer {
  if (_nativeGesturePanRecognizer) {
    _nativeGesturePanRecognizer.delegate = nil;
    [self removeGestureRecognizer:_nativeGesturePanRecognizer];
    _nativeGesturePanRecognizer = nil;
  }
}

- (void)dealloc {
  [self removeNativeGestureRecognizer];
}

@end
