// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxEventHandler+Internal.h>
#import <XElement/LynxOverlayContainer.h>

@interface LynxOverlaySimultaneouslyGestureRecognizer
    : UIGestureRecognizer <UIGestureRecognizerDelegate>
@property(nonatomic, weak) id<LynxOverlayViewDelegate> uiDelegate;
@end

@implementation LynxOverlaySimultaneouslyGestureRecognizer

- (instancetype)initWithTarget:(id)target action:(SEL)action {
  if (self = [super initWithTarget:target action:action]) {
    self.delegate = self;
    self.delaysTouchesBegan = NO;
    self.cancelsTouchesInView = NO;
    self.delaysTouchesEnded = NO;
  }
  return self;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  return YES;
}

- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)preventingGestureRecognizer {
  return NO;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [self.uiDelegate overlayTouched:@"overlaytouch"
                            point:[[touches allObjects].firstObject locationInView:self.view]
                            state:UIGestureRecognizerStateBegan
                         velocity:CGPointZero];
  self.state = UIGestureRecognizerStateBegan;
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [self.uiDelegate overlayTouched:@"overlaytouch"
                            point:[[touches allObjects].firstObject locationInView:self.view]
                            state:UIGestureRecognizerStateChanged
                         velocity:CGPointZero];
  self.state = UIGestureRecognizerStateChanged;
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [self.uiDelegate overlayTouched:@"overlaytouch"
                            point:[[touches allObjects].firstObject locationInView:self.view]
                            state:UIGestureRecognizerStateEnded
                         velocity:CGPointZero];
  self.state = UIGestureRecognizerStateEnded;
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
  [self.uiDelegate overlayTouched:@"overlaytouch"
                            point:[[touches allObjects].firstObject locationInView:self.view]
                            state:UIGestureRecognizerStateCancelled
                         velocity:CGPointZero];
  self.state = UIGestureRecognizerStateCancelled;
}

@end

/**
 * LynxOverlayContainer is designed to recognize touch events and resolve gesture conflicts.
 * Overlay has its pan gesture recognizer, and will be triggered  if :
 *  1. touch begins outside any scrollview
 *  2. touch begins inside the nested scrollview, and the scrollview is at the top, and the gesture
 * will move upwards.
 */
@interface LynxOverlayContainer ()
@property(nonatomic, strong) LynxOverlaySimultaneouslyGestureRecognizer *simulGesture;
@property(nonatomic, assign) NSInteger eventHandlerIndex;
@end

@implementation LynxOverlayContainer

- (void)enableTouchOverlayEvent:(BOOL)enable {
  if (enable) {
    if (!self.simulGesture) {
      self.simulGesture = [[LynxOverlaySimultaneouslyGestureRecognizer alloc]
          initWithTarget:self
                  action:@selector(handleSimulGesture:)];
      self.simulGesture.uiDelegate = self.uiDelegate;
    }
    self.simulGesture.enabled = !self.hidden;
    [self.window addGestureRecognizer:self.simulGesture];
  } else if (!enable) {
    [self.simulGesture.view removeGestureRecognizer:self.simulGesture];
    self.simulGesture = nil;
  }
}

- (void)willMoveToWindow:(UIWindow *)newWindow {
  [super willMoveToWindow:newWindow];
  if (newWindow && newWindow != self.simulGesture.view && self.simulGesture) {
    [self.simulGesture.view removeGestureRecognizer:self.simulGesture];
    [newWindow addGestureRecognizer:self.simulGesture];
  } else {
    [self.simulGesture.view removeGestureRecognizer:self.simulGesture];
  }
}

- (void)setHidden:(BOOL)hidden {
  [super setHidden:hidden];
  self.simulGesture.enabled = !hidden;
  if (self.eventHandler != nil) {
    if (hidden) {
      [self.eventHandler removeGestureArenaManager:_eventHandlerIndex];
      _eventHandlerIndex = -1;
    } else {
      LynxUI *rootUI = self.uiDelegate.overlayRootUI;
      if (_eventHandlerIndex < 0 && rootUI) {
        _eventHandlerIndex = [self.eventHandler
            setGestureArenaManagerAndGetIndex:rootUI.context.eventHandler.gestureArenaManager];
      }
    }
  }
}

- (instancetype)init {
  if (self = [super init]) {
    // edgePanRecognizer for swipe to exit page
    UIScreenEdgePanGestureRecognizer *edgePanRecognizer =
        [[UIScreenEdgePanGestureRecognizer alloc] initWithTarget:self
                                                          action:@selector(handleEdgePanGesture:)];
    edgePanRecognizer.edges = UIRectEdgeLeft;
    [self addGestureRecognizer:edgePanRecognizer];

    // panGesture to let lepus responds to user interaction
    UIPanGestureRecognizer *panGesture =
        [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanGesture:)];
    panGesture.delegate = self;
    [self addGestureRecognizer:panGesture];
    self.eventHandlerIndex = -1;
  }
  return self;
}

/**
 * Integrate Lynx event system
 */
- (void)ensureEventHandler {
  if (self.eventHandler != nil) {
    return;
  }
  LynxUI *rootUI = self.uiDelegate.overlayRootUI;
  BOOL flag = rootUI.context.lynxContext.isFragmentLayerRenderOn;
  self.eventHandler = [[LynxEventHandler alloc] initWithRootView:self
                                                      withRootUI:rootUI
                                                         andFlag:flag];
  [self.eventHandler updateUiOwner:rootUI.context.uiOwner eventEmitter:rootUI.context.eventEmitter];
  _eventHandlerIndex = [self.eventHandler
      setGestureArenaManagerAndGetIndex:rootUI.context.eventHandler.gestureArenaManager];
}

/**
 * Override hitTest to make sure eventPassThrough works
 * If no subview is hit, return nil to let event pass through, or return self to block event
 */
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  [self ensureEventHandler];
  UIView *view = [super hitTest:point withEvent:event];
  if (!view) {
    return nil;
  }
  if ([self.uiDelegate eventPassed:point] && view == self) {
    return nil;
  } else {
    // find touchTarget via lynx event system
    id<LynxEventTarget> touchTarget = [self.eventHandler hitTest:point withEvent:event];

    // handle focus
    [self.eventHandler handleFocus:touchTarget
                            onView:view
                     withContainer:self
                          andPoint:point
                          andEvent:event];

    // check if touchTarget can be event through
    CGPoint targetPoint = point;
    if ([touchTarget isKindOfClass:[LynxUI class]]) {
      targetPoint = [self convertPoint:point toView:((LynxUI *)touchTarget).view];
    }
    if ([touchTarget eventThrough:targetPoint]) {
      return [self.uiDelegate eventPassed:point] ? nil : self;
    } else {
      return view ?: ([self.uiDelegate eventPassed:point] ? nil : self);
    }
  }
}

/**
 * Swipe from edge to dismiss Overlay
 */
- (void)handleEdgePanGesture:(UIScreenEdgePanGestureRecognizer *)recognizer {
  [self.uiDelegate requestClose:@{}];
}

/**
 * Dispatch user interaction
 */
- (void)handlePanGesture:(UIPanGestureRecognizer *)recognizer {
  [self.uiDelegate overlayTouched:@"overlaymoved"
                            point:[recognizer locationInView:self]
                            state:recognizer.state
                         velocity:[recognizer velocityInView:self]];
}

- (void)handleSimulGesture:(UIPanGestureRecognizer *)recognizer {
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizerShouldBegin:(UIPanGestureRecognizer *)gestureRecognizer {
  if (gestureRecognizer.delegate == self) {
    CGPoint velocity = [gestureRecognizer velocityInView:self];
    UIScrollView *nestScrollView = [self.uiDelegate nestScrollView];
    // if there is no nested scrollview, pan gesture can be triggered
    if (!nestScrollView) {
      return ABS(velocity.x) < ABS(velocity.y);
    }
    CGPoint pointInScrollView = [gestureRecognizer locationInView:nestScrollView];
    CGPoint adjustedPoint = {pointInScrollView.x - nestScrollView.contentOffset.x,
                             pointInScrollView.y - nestScrollView.contentOffset.y};

    // check if touch begins inside the nested scrollview and if the nested scrollview is at its top
    if (nestScrollView.contentOffset.y > 0 && adjustedPoint.y >= 0 && adjustedPoint.x >= 0) {
      return NO;
    }

    // now, touch begins inside the nested scrollview, check if the gesture is vertical and will
    // move upwards
    return ABS(velocity.x) < ABS(velocity.y) &&
           (velocity.y > 0 || (adjustedPoint.y < 0 || adjustedPoint.x < 0));
  } else {
    return YES;
  }
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  // if the nested scrollview is at its top, we can not recognize both of them
  if (otherGestureRecognizer.view == [self.uiDelegate nestScrollView] &&
      [self.uiDelegate nestScrollView].contentOffset.y <= 0) {
    return NO;
  }
  // we can not trigger Overlay's gesture together with other scrollview's gesture inside the
  // `nest-scroll-view` if it can be scrolled
  if ([otherGestureRecognizer.view isDescendantOfView:[self.uiDelegate nestScrollView]]) {
    if ([self checkScrollViewCanBeScrolled:(UIScrollView *)otherGestureRecognizer.view
                            withPanGesture:(UIPanGestureRecognizer *)otherGestureRecognizer]) {
      return NO;
    }
  }
  return YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
  // check if swipe back gesture of Lynx's container should be recognized
  if ([otherGestureRecognizer.view isKindOfClass:NSClassFromString(@"UILayoutContainerView")]) {
    return [self.uiDelegate forbidPanGesture];
  }
  // if the nested scrollview is at its top, the scrollview can be recognized only if Overlay's
  // gesture failed.
  if (otherGestureRecognizer.view == [self.uiDelegate nestScrollView] &&
      [self.uiDelegate nestScrollView].contentOffset.y <= 0) {
    return YES;
  }
  // if a scrollview inside the `nest-scroll-view` can be scrolled, it can only be triggered after
  // overlay's vertical gesture failed
  if ([otherGestureRecognizer.view isDescendantOfView:[self.uiDelegate nestScrollView]]) {
    if ([self checkScrollViewCanBeScrolled:(UIScrollView *)otherGestureRecognizer.view
                            withPanGesture:(UIPanGestureRecognizer *)otherGestureRecognizer]) {
      return YES;
    }
  }
  return NO;
}

/**
 * Check if a scroll view is able to scroll in a overlay container.
 * Notice: this scroll view can not be the nested one
 */
- (BOOL)checkScrollViewCanBeScrolled:(UIScrollView *)scrollView
                      withPanGesture:(UIPanGestureRecognizer *)gestureRecognizer {
  if (![scrollView isKindOfClass:UIScrollView.class] ||
      ![gestureRecognizer isKindOfClass:UIPanGestureRecognizer.class]) {
    return NO;
  }
  if (scrollView.scrollEnabled) {
    if (scrollView.contentSize.height > scrollView.bounds.size.height ||
        scrollView.alwaysBounceVertical) {
      // a vertical scrollview, allow to be scrolled
      return YES;
    } else if (scrollView.contentSize.width > scrollView.bounds.size.width ||
               scrollView.alwaysBounceHorizontal) {
      // a horizontal scrollview, check if the gesture is horizontal
      CGPoint velocity = [gestureRecognizer velocityInView:self];
      if (ABS(velocity.x) >= ABS(velocity.y)) {
        return YES;
      }
    }
  }
  return NO;
}

- (BOOL)gestureRecognizer:(__unused UIGestureRecognizer *)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
  // check if swipe back gesture of Lynx's container should be recognized
  if ([otherGestureRecognizer.view isKindOfClass:NSClassFromString(@"UILayoutContainerView")]) {
    return ![self.uiDelegate forbidPanGesture];
  }
  return NO;
}

@end
