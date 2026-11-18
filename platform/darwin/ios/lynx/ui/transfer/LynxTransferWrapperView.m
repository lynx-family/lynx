// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTransferWrapperView.h>

#import <Lynx/LynxEvent.h>
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxEventHandler.h>

#import "LynxUITransfer.h"

@interface LynxUITransfer (LynxTransferWrapperView)

- (void)syncHostConstraintsFromWrapper:(LynxTransferWrapperView*)wrapperView;
- (CGSize)transferContentSize;
- (void)dispatchIntrinsicContentSizeChange;

@property(nonatomic, strong, readonly, nullable) LynxEventHandler* transferEventHandler;

@end

@interface LynxTransferWrapperView ()

@property(nonatomic, weak, readonly) LynxUITransfer* transfer;
@property(nonatomic, assign) CGSize measuredContentSize;
@property(nonatomic, assign) BOOL hasLayoutConstraints;

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
  if (self.enableAutoLayout && !CGSizeEqualToSize(self.bounds.size, CGSizeZero)) {
    self.layoutWidthMode = LynxViewSizeModeExact;
    self.layoutHeightMode = LynxViewSizeModeExact;
    self.preferredLayoutWidth = self.bounds.size.width;
    self.preferredLayoutHeight = self.bounds.size.height;
    [self triggerLayout];
  }
}

- (void)setLayoutWidthMode:(LynxViewSizeMode)mode {
  _layoutWidthMode = mode;
  self.hasLayoutConstraints = YES;
}

- (void)setLayoutHeightMode:(LynxViewSizeMode)mode {
  _layoutHeightMode = mode;
  self.hasLayoutConstraints = YES;
}

- (void)didMoveToSuperview {
  [super didMoveToSuperview];
  // Seed the initial viewport from a nonzero host frame, as LynxView does at setup.
  // Later explicit modes survive frame assignment and reparenting.
  if (!self.hasLayoutConstraints && self.superview != nil && self.superview != self.transfer.view &&
      !CGSizeEqualToSize(self.bounds.size, CGSizeZero)) {
    self.layoutWidthMode = LynxViewSizeModeExact;
    self.layoutHeightMode = LynxViewSizeModeExact;
    self.preferredLayoutWidth = self.bounds.size.width;
    self.preferredLayoutHeight = self.bounds.size.height;
    [self triggerLayout];
  }
}

- (void)triggerLayout {
  self.hasLayoutConstraints = YES;
  [self.transfer syncHostConstraintsFromWrapper:self];
}

- (CGSize)intrinsicContentSize {
  return self.measuredContentSize;
}

// Called from the existing Lynx layout/operation completion hooks, after child frames update.
- (void)layoutDidFinished {
  CGSize size = [self.transfer transferContentSize];
  if (self.layoutWidthMode == LynxViewSizeModeExact) {
    size.width = self.preferredLayoutWidth;
  } else if (self.layoutWidthMode == LynxViewSizeModeMax) {
    size.width = MIN(size.width, self.preferredMaxLayoutWidth);
  }
  if (self.layoutHeightMode == LynxViewSizeModeExact) {
    size.height = self.preferredLayoutHeight;
  } else if (self.layoutHeightMode == LynxViewSizeModeMax) {
    size.height = MIN(size.height, self.preferredMaxLayoutHeight);
  }
  if (!CGSizeEqualToSize(size, self.measuredContentSize)) {
    self.measuredContentSize = size;
    [self.transfer dispatchIntrinsicContentSizeChange];
  }
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
