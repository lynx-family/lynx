// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxShadowNode.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext.h>

#import "../../shadow_node/transfer/LynxTransferShadowNode.h"
#import "LynxTransferHostView.h"
#import "LynxTransferWrapperView.h"
#import "LynxUITransfer.h"

@interface LynxUITransfer ()

@property(nonatomic, strong) LynxTransferWrapperView* wrapperView;

- (void)syncHostConstraintsFromWrapper:(LynxTransferWrapperView*)wrapperView;

@end

@implementation LynxUITransfer

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("transfer-view")
#else
LYNX_REGISTER_UI("transfer-view")
#endif

- (UIView*)createView {
  LynxTransferHostView* hostView = [[LynxTransferHostView alloc] init];
  hostView.translatesAutoresizingMaskIntoConstraints = YES;
  hostView.hidden = YES;

  self.wrapperView = [[LynxTransferWrapperView alloc] initWithTransfer:self];
  self.wrapperView.frame = hostView.bounds;
  [hostView addSubview:self.wrapperView];
  return hostView;
}

- (UIView*)childrenContainerView {
  return self.wrapperView ?: self.view;
}

- (BOOL)isVisible {
  return !self.wrapperView.hidden && self.wrapperView.alpha >= 0.01;
}

- (void)syncHostConstraintsFromWrapper:(LynxTransferWrapperView*)wrapperView {
  BOOL hasExternalHost = wrapperView.superview != nil && wrapperView.superview != self.view;
  CGFloat width = CGRectGetWidth(wrapperView.bounds);
  CGFloat height = CGRectGetHeight(wrapperView.bounds);
  LynxMeasureMode widthMode = hasExternalHost ? LynxMeasureModeDefinite : LynxMeasureModeIndefinite;
  LynxMeasureMode heightMode =
      hasExternalHost ? LynxMeasureModeDefinite : LynxMeasureModeIndefinite;
  [self.context findShadowNodeAndRunTask:self.sign
                                    task:^(LynxShadowNode* node) {
                                      if ([node isKindOfClass:[LynxTransferShadowNode class]]) {
                                        [(LynxTransferShadowNode*)node
                                            updateHostConstraintsWithWidth:width
                                                                 widthMode:widthMode
                                                                    height:height
                                                                heightMode:heightMode];
                                      }
                                    }];
}

@end
