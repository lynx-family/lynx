// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIView.h"

#import "LynxComponentRegistry.h"
#import "LynxUI+Internal.h"

@interface UILynxView : UIView

@end

@implementation UILynxView

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  if (!self.isUserInteractionEnabled || self.isHidden || self.alpha <= 0.01) {
    return nil;
  }

  if (self.clipsToBounds) {
    return [super hitTest:point withEvent:event];
  }
  CAShapeLayer* mask = nil;
  CGAffineTransform transform = CGAffineTransformIdentity;
  if ([self.layer.mask isKindOfClass:[CAShapeLayer class]]) {
    mask = (CAShapeLayer*)self.layer.mask;
    transform = mask.affineTransform;
  }
  __block UIView* target = nil;
  [self.subviews enumerateObjectsWithOptions:NSEnumerationReverse
                                  usingBlock:^(__kindof UIView* _Nonnull obj, NSUInteger idx,
                                               BOOL* _Nonnull stop) {
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

- (UIView*)createView {
  UIView* view = [[UILynxView alloc] init];
  // Disable AutoLayout
  [view setTranslatesAutoresizingMaskIntoConstraints:YES];
  return view;
}

- (BOOL)enableAccessibilityByDefault {
  return NO;
}

- (void)setContext:(LynxUIContext*)context {
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
