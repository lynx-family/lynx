// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTransferWrapperView.h"

#import <Lynx/LynxEvent.h>
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxEventHandler.h>

#import "LynxUITransfer.h"

@interface LynxUITransfer (LynxTransferWrapperView)

- (void)syncHostConstraintsFromWrapper:(LynxTransferWrapperView*)wrapperView;

@property(nonatomic, strong, readonly, nullable) LynxEventHandler* transferEventHandler;

@end

@interface LynxTransferWrapperView ()

@property(nonatomic, weak, readonly) LynxUITransfer* transfer;

@end

@implementation LynxTransferWrapperView

- (instancetype)initWithTransfer:(LynxUITransfer*)transfer {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _transfer = transfer;
    self.translatesAutoresizingMaskIntoConstraints = YES;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self.transfer syncHostConstraintsFromWrapper:self];
}

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  if (!self.isUserInteractionEnabled || self.isHidden || self.alpha <= 0.01) {
    return nil;
  }

  LynxEventHandler* eventHandler = self.transfer.transferEventHandler;
  // use the EventHandler of UITransfer to find the LynxUI which response to this event
  id<LynxEventTarget> touchTarget = [eventHandler hitTest:point withEvent:event];
  CGPoint targetPoint = point;
  UIView* view = nil;
  if (touchTarget == self.transfer) {
    view = [super hitTest:point withEvent:event];
  } else if (touchTarget.view) {
    targetPoint = [self convertPoint:point toView:touchTarget.view];
    // The UIView of touchTarget may have children, so we need to find the final hitted view with
    // the UIView.
    view = [touchTarget.view hitTest:targetPoint withEvent:event] ?: touchTarget.view;
  }
  if (view == nil) {
    view = [super hitTest:point withEvent:event];
  }

  [eventHandler handleFocus:touchTarget
                     onView:view
              withContainer:self
                   andPoint:point
                   andEvent:event];
  return [touchTarget eventThrough:targetPoint] ? nil : view;
}

@end
