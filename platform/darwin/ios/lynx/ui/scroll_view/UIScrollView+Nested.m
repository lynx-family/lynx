// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "UIScrollView+Nested.h"

@interface LynxWeakParentScrollViewWrapper : NSObject
@property(nonatomic, weak) UIScrollView* parentScrollView;
@end

@implementation LynxWeakParentScrollViewWrapper
- (instancetype)initWithParentScrollView:(UIScrollView*)parentScrollView {
  if (!(self = [super init])) {
    return nil;
  }
  _parentScrollView = parentScrollView;
  return self;
}
@end

@implementation UIScrollView (Nested)

#pragma mark get methods

- (UIScrollView*)parentScrollView {
  LynxWeakParentScrollViewWrapper* parentScrollViewWrapper = objc_getAssociatedObject(self, _cmd);
  return parentScrollViewWrapper.parentScrollView;
}

- (NSPointerArray*)childrenScrollView {
  return objc_getAssociatedObject(self, _cmd);
}

- (BOOL)enableNested {
  return [objc_getAssociatedObject(self, @selector(enableNested)) boolValue];
}

- (BOOL)isRTL {
  return [objc_getAssociatedObject(self, @selector(isRTL)) boolValue];
}

- (BOOL)scrollY {
  return [objc_getAssociatedObject(self, @selector(scrollY)) boolValue];
}

- (CGPoint)lastPosition {
  return [(NSValue*)objc_getAssociatedObject(self, @selector(lastPosition)) CGPointValue];
}

- (NSString*)name {
  return (NSString*)objc_getAssociatedObject(self, @selector(name));
}

#pragma mark set methods

- (void)setParentScrollView:(UIScrollView*)scrollView {
  LynxWeakParentScrollViewWrapper* parentScrollViewWrapper =
      [[LynxWeakParentScrollViewWrapper alloc] initWithParentScrollView:scrollView];
  objc_setAssociatedObject(self, @selector(parentScrollView), parentScrollViewWrapper,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (void)setChildrenScrollView:(NSPointerArray*)childrenScrollView {
  objc_setAssociatedObject(self, @selector(childrenScrollView), childrenScrollView,
                           OBJC_ASSOCIATION_RETAIN);
}

- (void)setLastPosition:(CGPoint)position {
  // considering bounce occasion, the over-edge-distance while bouncing should be constrained.
  CGPoint constrainedPoint;
  if (self.scrollY) {
    CGFloat constrainedY = MIN(position.y, self.contentSize.height - self.frame.size.height);
    constrainedY = MAX(constrainedY, 0);
    constrainedPoint = CGPointMake(self.lastPosition.x, constrainedY);
  } else {
    CGFloat constrainedX = MIN(position.x, self.contentSize.width - self.frame.size.width);
    constrainedX = MAX(constrainedX, 0);
    constrainedPoint = CGPointMake(constrainedX, self.lastPosition.y);
  }
  objc_setAssociatedObject(self, @selector(lastPosition),
                           [NSValue valueWithCGPoint:constrainedPoint], OBJC_ASSOCIATION_RETAIN);
}

- (void)setEnableNested:(BOOL)enable {
  objc_setAssociatedObject(self, @selector(enableNested), [NSNumber numberWithBool:enable],
                           OBJC_ASSOCIATION_RETAIN);
}

- (void)setIsRTL:(BOOL)isRTL {
  objc_setAssociatedObject(self, @selector(isRTL), [NSNumber numberWithBool:isRTL],
                           OBJC_ASSOCIATION_RETAIN);
}

- (void)setScrollY:(BOOL)isScrollY {
  objc_setAssociatedObject(self, @selector(scrollY), [NSNumber numberWithBool:isScrollY],
                           OBJC_ASSOCIATION_RETAIN);
}

- (void)setName:(NSString*)viewName {
  objc_setAssociatedObject(self, @selector(name), viewName, OBJC_ASSOCIATION_RETAIN);
}

#pragma mark methods

- (BOOL)isOverEdge:(BOOL)isScrollY {
  CGPoint translation = [self.panGestureRecognizer translationInView:self];
  if (!isScrollY) {
    if (translation.x > 0 && self.contentOffset.x <= 0) {
      return YES;
    }
    CGFloat maxOffsetX = floor(self.contentSize.width - self.bounds.size.width);
    if (translation.x < 0 && self.contentOffset.x >= maxOffsetX) {
      return YES;
    }
    return NO;
  }

  if (translation.y > 0 && self.contentOffset.y <= 0) {
    return YES;
  }
  CGFloat maxOffsetY = floor(self.contentSize.height - self.bounds.size.height);
  if (translation.y < 0 && self.contentOffset.y >= maxOffsetY) {
    return YES;
  }
  return NO;
}

- (UIScrollView*)nearestParentScrollView {
  UIScrollView* currentView = self;
  while (currentView.superview) {
    if ([currentView.superview respondsToSelector:@selector(enableNested)]) {
      UIScrollView* parentScrollView = (UIScrollView*)currentView.superview;
      if (parentScrollView.enableNested && parentScrollView.scrollY == self.scrollY) {
        return parentScrollView;
      } else {
        currentView = (UIScrollView*)currentView.superview;
      }
    } else {
      currentView = (UIScrollView*)currentView.superview;
    }
  }
  return nil;
}

- (void)updateChildren {
  if (self.enableNested) {
    UIScrollView* nearestParent = [self nearestParentScrollView];
    if (nearestParent && [nearestParent isKindOfClass:[UIScrollView class]]) {
      self.bounces = NO;
      if (!nearestParent.childrenScrollView) {
        nearestParent.childrenScrollView = [NSPointerArray weakObjectsPointerArray];
      }
      NSPointerArray* children = nearestParent.childrenScrollView;
      [children addPointer:(__bridge void* _Nullable)self];
      self.parentScrollView = nearestParent;
    }
  }
}

- (BOOL)childScrollViewCanScrollAtPoint:(CGPoint)point withDirection:(BOOL)isScrollY {
  int length = (int)self.childrenScrollView.count;
  for (int i = length - 1; i >= 0; i--) {
    UIScrollView* currentChild = (UIScrollView*)[self.childrenScrollView pointerAtIndex:i];
    BOOL contained = [currentChild.layer containsPoint:[self convertPoint:point
                                                                   toView:currentChild]];
    BOOL canScroll = ![currentChild isOverEdge:isScrollY];
    // If the user swipes outside scrollview within one gesture, the isDragging flag should be
    // considered to stop parentScrollView from scrolling
    if ((contained || currentChild.isDragging) && canScroll) {
      return YES;
    }
  }
  return NO;
}

- (void)triggerNestedScrollView:(BOOL)enableScrollY {
  if (self.enableNested) {
    UIPanGestureRecognizer* panGestureRecognizer = self.panGestureRecognizer;
    // if children can scroll, fix itself on the previous position.
    if (self.childrenScrollView) {
      if ([self childScrollViewCanScrollAtPoint:[panGestureRecognizer locationInView:self]
                                  withDirection:enableScrollY]) {
        [self setContentOffset:self.lastPosition];
      } else {
        self.lastPosition = self.contentOffset;
      }
      // if it has parent, fix itself when it reaches the edge
    } else if (self.parentScrollView) {
      if (![self isOverEdge:enableScrollY]) {
        self.lastPosition = self.contentOffset;
      } else {
        if (enableScrollY) {
          // if over the edge, set the offset back to edge
          // scrollY
          CGFloat maxOffsetY = floor(self.contentSize.height - self.bounds.size.height);
          if (self.contentOffset.y <= 0) {
            [self setContentOffset:CGPointMake(0, 0)];
          } else if (self.contentOffset.y >= maxOffsetY) {
            [self setContentOffset:CGPointMake(0, maxOffsetY)];
          } else {
            [self setContentOffset:self.lastPosition];
          }
        } else {
          // scrollX
          CGFloat maxOffsetX = floor(self.contentSize.width - self.bounds.size.width);
          if (self.contentOffset.x <= 0) {
            [self setContentOffset:CGPointMake(0, 0)];
          } else if (self.contentOffset.x >= maxOffsetX) {
            [self setContentOffset:CGPointMake(maxOffsetX, 0)];
          } else {
            [self setContentOffset:self.lastPosition];
          }
        }
      }
    }
  }
}

@end
