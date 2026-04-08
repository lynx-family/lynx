// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxUIScrollCoordinatorSlot.h>

@interface LynxUIScrollCoordinatorSlot ()
@end

@implementation LynxUIScrollCoordinatorSlot

#pragma mark - LynxUI

- (UIView *)createView {
  return [[UIView alloc] init];
}

- (void)insertChild:(id)child atIndex:(NSInteger)index {
  [super insertChild:child atIndex:index];
  if ([child isKindOfClass:LynxUIScrollCoordinatorSlotDrag.class]) {
    self.slotDrag = child;
  }
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];
  self.slotHeightChanged = YES;
}

- (BOOL)notifyParent {
  return YES;
}

- (CGRect)getHitTestFrameWithFrame:(CGRect)frame {
  frame = self.view.frame;
  if ([self.view.superview isKindOfClass:[UIScrollView class]]) {
    frame.origin.y -= ((UIScrollView *)self.view.superview).contentOffset.y;
  }
  // frame should calculate translate and scale
  float scaleX = self.scaleX;
  float scaleY = self.scaleY;
  float centerX = frame.origin.x + frame.size.width / 2.0f;
  float centerY = frame.origin.y + frame.size.height / 2.0f;
  float rectX = centerX - frame.size.width * scaleX / 2.0f + self.getTransationX;
  float rectY = centerY - frame.size.height * scaleY / 2.0f + self.getTransationY;
  CGRect transFrame =
      CGRectMake(rectX, rectY, frame.size.width * scaleX, frame.size.height * scaleY);

  if (transFrame.size.width + self.hitSlopLeft + self.hitSlopRight >= CGFLOAT_EPSILON &&
      transFrame.size.height + self.hitSlopTop + self.hitSlopBottom >= CGFLOAT_EPSILON) {
    transFrame.origin.x -= self.hitSlopLeft;
    transFrame.origin.y -= self.hitSlopTop;
    transFrame.size.width += self.hitSlopLeft + self.hitSlopRight;
    transFrame.size.height += self.hitSlopTop + self.hitSlopBottom;
  }

  return transFrame;
}

@end
