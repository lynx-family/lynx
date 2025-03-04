// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#import "LynxLazyLoad.h"
#import "UIScrollView+LynxFadingEdge.h"

@interface LynxFadingEdge : NSObject

@property(nonatomic, strong) CAGradientLayer *fadingEdgeLayer;
@property(nonatomic, assign) CGFloat size;
@property(nonatomic, assign) BOOL horizontal;

@end

@implementation LynxFadingEdge

+ (void)swizzle:(Class)class instanceMethod:(SEL)originalSelector with:(SEL)swizzledSelector {
  Method originalMethod = class_getInstanceMethod(class, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(class, swizzledSelector);

  BOOL didAddMethod =
      class_addMethod(class, originalSelector, method_getImplementation(swizzledMethod),
                      method_getTypeEncoding(swizzledMethod));

  if (didAddMethod) {
    class_replaceMethod(class, swizzledSelector, method_getImplementation(originalMethod),
                        method_getTypeEncoding(originalMethod));
  } else {
    method_exchangeImplementations(originalMethod, swizzledMethod);
  }
}

@end

@implementation UIScrollView (LynxFadingEdge)

LYNX_LOAD_LAZY([LynxFadingEdge swizzle:self
                        instanceMethod:@selector(setContentOffset:)
                                  with:@selector(fadingSetContentOffset:)];
               [LynxFadingEdge swizzle:self
                        instanceMethod:@selector(setContentSize:)
                                  with:@selector(fadingSetContentSize:)];)

/*
 * Update the fading edge if necessary:
 *  1. If no size, remove it.
 *  2. If there is no fading edge, create it with size and direction.
 *  3. If there is any difference of the size or the direction, update it.
 */
- (void)updateFadingEdgeWithSize:(CGFloat)size horizontal:(BOOL)horizontal {
  LynxFadingEdge *fadingEdge = self.fadingEdge;
  if (size <= 0.0) {
    if (fadingEdge) {
      self.fadingEdge = nil;
      self.layer.mask = nil;
    }
  } else {
    if (!fadingEdge) {
      [self setupFadingEdgeWithSize:size horizontal:horizontal];
    } else if (size != fadingEdge.size || horizontal != fadingEdge.horizontal) {
      fadingEdge.size = size;
      fadingEdge.horizontal = horizontal;
      [self updateLayer:fadingEdge.fadingEdgeLayer size:size horizontal:horizontal];
    }
  }
}

/*
 * Create a mask layer of opacity for UIScrollView to make it look like `fading`
 */
- (void)setupFadingEdgeWithSize:(CGFloat)size horizontal:(BOOL)horizontal {
  LynxFadingEdge *fadingEdge = [[LynxFadingEdge alloc] init];
  fadingEdge.size = size;
  fadingEdge.horizontal = horizontal;

  CAGradientLayer *maskLayer = [CAGradientLayer layer];
  CGColorRef innerColor = [UIColor.whiteColor colorWithAlphaComponent:1.0].CGColor;
  CGColorRef outerColor = [UIColor.whiteColor colorWithAlphaComponent:0.0].CGColor;

  // Apply color at 4 gradient stops, to mask the UIScrollView both at the start and the end.
  maskLayer.colors = @[
    (__bridge id)outerColor, (__bridge id)innerColor, (__bridge id)innerColor,
    (__bridge id)outerColor
  ];

  maskLayer.anchorPoint = CGPointZero;
  self.layer.mask = maskLayer;
  fadingEdge.fadingEdgeLayer = maskLayer;
  self.fadingEdge = fadingEdge;
  [self updateLayer:maskLayer size:size horizontal:horizontal];
}

/*
 * Update the maskLayer according to the size, horizontal and the status of UIScrollView
 */
- (void)updateLayer:(CAGradientLayer *)layer size:(CGFloat)size horizontal:(BOOL)horizontal {
  // Disable implicit animation for CALayer.
  [CATransaction begin];
  [CATransaction setDisableActions:YES];

  if (horizontal) {
    CGFloat percent = size / self.frame.size.width;
    // If we are approaching to the left or the right of scrollview, the fading edge will begin to
    // `shrink` from size to zero. So, we should take the contentOffset into account.
    CGFloat startAlphaOffset =
        MIN(1.0, MAX(0.0, (self.contentOffset.x + self.contentInset.left) / size)) * percent;
    CGFloat endAlphaOffset =
        MIN(1.0,
            MAX(0.0, (self.contentSize.width - (self.contentOffset.x + self.contentInset.left) -
                      self.frame.size.width) /
                         size)) *
        percent;
    layer.locations = @[ @(0.0), @(startAlphaOffset), @(1 - endAlphaOffset), @(1.0) ];

    layer.frame =
        CGRectMake(self.contentOffset.x, 0, self.frame.size.width, self.frame.size.height);
    layer.startPoint = CGPointMake(0, 0);
    layer.endPoint = CGPointMake(1, 0);
  } else {
    CGFloat percent = size / self.frame.size.width;
    CGFloat startAlphaOffset =
        MIN(1.0, MAX(0.0, (self.contentOffset.y + self.contentInset.top) / size)) * percent;
    CGFloat endAlphaOffset =
        MIN(1.0,
            MAX(0.0, (self.contentSize.height - (self.contentOffset.y + self.contentInset.top) -
                      self.frame.size.height) /
                         size)) *
        percent;
    layer.locations = @[ @(0.0), @(startAlphaOffset), @(1 - endAlphaOffset), @(1.0) ];
    layer.frame =
        CGRectMake(0, self.contentOffset.y, self.frame.size.width, self.frame.size.height);
    layer.startPoint = CGPointMake(0, 0);
    layer.endPoint = CGPointMake(0, 1);
  }
  [CATransaction commit];
}

/*
 * Hook `setContentSize:` to updateLayer
 */
- (void)fadingSetContentSize:(CGSize)size {
  [self fadingSetContentSize:size];
  LynxFadingEdge *fadingEdge = self.fadingEdge;
  if (fadingEdge) {
    [self updateLayer:fadingEdge.fadingEdgeLayer
                 size:fadingEdge.size
           horizontal:fadingEdge.horizontal];
  }
}

/*
 * Hook `setContentOffset:` to updateLayer
 */
- (void)fadingSetContentOffset:(CGPoint)contentOffset {
  [self fadingSetContentOffset:contentOffset];
  LynxFadingEdge *fadingEdge = self.fadingEdge;
  if (fadingEdge) {
    [self updateLayer:fadingEdge.fadingEdgeLayer
                 size:fadingEdge.size
           horizontal:fadingEdge.horizontal];
  }
}

- (LynxFadingEdge *)fadingEdge {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setFadingEdge:(LynxFadingEdge *)edge {
  objc_setAssociatedObject(self, @selector(fadingEdge), edge, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end
