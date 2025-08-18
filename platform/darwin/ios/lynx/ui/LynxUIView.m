// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIView.h>

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/UIScrollView+LynxGesture.h>

@interface UILynxView : UIView <UIGestureRecognizerDelegate>

@property(nonatomic, strong) UIPanGestureRecognizer *nativeGesturePanRecognizer;

@property(nonatomic, assign) LynxInterceptGestureState interceptGestureStatus;

@end

@implementation UILynxView

- (void)setupNativeGestureRecognizerIfNeeded {
  if (_nativeGesturePanRecognizer == nil) {
    _nativeGesturePanRecognizer =
        [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanGesture:)];
    _nativeGesturePanRecognizer.delegate = self;
    [self addGestureRecognizer:_nativeGesturePanRecognizer];
  }
}

- (void)handlePanGesture:(UIPanGestureRecognizer *)recognizer {
  if (self.interceptGestureStatus == LynxInterceptGestureStateTrue) {
    // Intercept the gesture without handling, so it will be consumed
    return;
  }
  recognizer.state = UIGestureRecognizerStateCancelled;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  if (gestureRecognizer == self.nativeGesturePanRecognizer &&
      self.interceptGestureStatus == LynxInterceptGestureStateTrue) {
    otherGestureRecognizer.state = UIGestureRecognizerStateFailed;
    return NO;
  }
  return YES;
}

// Controls whether gesture recognition should begin
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
  if (gestureRecognizer == self.nativeGesturePanRecognizer) {
    // When interceptGesture is true, return YES to consume this gesture
    // When interceptGesture is false or unset, return NO to allow gesture passing
    return self.interceptGestureStatus == LynxInterceptGestureStateTrue;
  }
  return YES;
}

- (void)dealloc {
  // Remove the gesture recognizer from the view
  if (_nativeGesturePanRecognizer) {
    _nativeGesturePanRecognizer.delegate = nil;
    [self removeGestureRecognizer:_nativeGesturePanRecognizer];
    _nativeGesturePanRecognizer = nil;
  }
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  if (!self.isUserInteractionEnabled || self.isHidden || self.alpha <= 0.01) {
    return nil;
  }

  if (self.clipsToBounds) {
    return [super hitTest:point withEvent:event];
  }
  CAShapeLayer *mask = nil;
  CGAffineTransform transform = CGAffineTransformIdentity;
  if ([self.layer.mask isKindOfClass:[CAShapeLayer class]]) {
    mask = (CAShapeLayer *)self.layer.mask;
    transform = mask.affineTransform;
  }
  __block UIView *target = nil;
  [self.subviews enumerateObjectsWithOptions:NSEnumerationReverse
                                  usingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                               BOOL *_Nonnull stop) {
                                    if (mask == nil ||
                                        CGPathContainsPoint(mask.path, &transform, point, false)) {
                                      CGPoint newPoint = [obj convertPoint:point fromView:self];
                                      target = [obj hitTest:newPoint withEvent:event];
                                      if (target != nil) {
                                        *stop = YES;
                                      }
                                    }
                                  }];
  return target == nil ? [super hitTest:point withEvent:event] : target;
}

@end

@implementation LynxUIView

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("view")
#else
LYNX_REGISTER_UI("view")
#endif

- (UIView *)createView {
  UIView *view = [[UILynxView alloc] init];
  // Disable AutoLayout
  [view setTranslatesAutoresizingMaskIntoConstraints:YES];
  return view;
}

- (BOOL)enableAccessibilityByDefault {
  return NO;
}

- (void)gestureDidSet {
  [super gestureDidSet];
  for (NSNumber *key in self.gestureMap) {
    LynxGestureDetectorDarwin *detector = self.gestureMap[key];
    // Check if a native gesture type exists.
    if (detector.gestureType == LynxGestureTypeNative) {
      // If found, ensure the native pan recognizer is set up on the underlying view.
      UILynxView *lynxView = (UILynxView *)self.view;
      if ([lynxView isKindOfClass:[UILynxView class]]) {
        [lynxView setupNativeGestureRecognizerIfNeeded];
      }
      break;
    }
  }
}

- (void)interceptGesture:(BOOL)intercept {
  UILynxView *lynxView = (UILynxView *)self.view;
  if (intercept) {
    lynxView.interceptGestureStatus = LynxInterceptGestureStateTrue;
  } else {
    lynxView.interceptGestureStatus = LynxInterceptGestureStateFalse;
  }
}

- (void)setContext:(LynxUIContext *)context {
  [super setContext:context];
  if (self.context.defaultOverflowVisible) {
    self.overflow = OVERFLOW_XY_VAL;
  } else {
    self.view.clipsToBounds = YES;
  }
}

- (LynxOverflowType)getInitialOverflowType {
  return self.context.defaultOverflowVisible ? LynxOverflowVisible : LynxOverflowHidden;
}

@end
