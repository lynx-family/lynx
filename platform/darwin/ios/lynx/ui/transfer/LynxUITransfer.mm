// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxShadowNode.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxView.h>

#import "../../shadow_node/transfer/LynxTransferShadowNode.h"
#import "LynxTransferHostView.h"
#import "LynxTransferWrapperView.h"
#import "LynxUITransfer.h"

@interface LynxView ()

- (void)dispatchTransferCreate:(NSString*)transferId
                          view:(UIView*)view
                       dataset:(NSDictionary*)dataset;
- (void)dispatchTransferDatasetUpdate:(UIView*)view dataset:(NSDictionary*)dataset;
- (void)dispatchTransferRemove:(NSString*)transferId view:(UIView*)view;

@end

@interface LynxUITransfer ()

@property(nonatomic, strong) LynxTransferWrapperView* wrapperView;
@property(nonatomic, copy, nullable) NSString* transferId;
@property(nonatomic, copy, nullable) NSString* attachedTransferId;
@property(nonatomic, assign) BOOL nodeReady;

- (void)syncHostConstraintsFromWrapper:(LynxTransferWrapperView*)wrapperView;
- (void)cleanupTransfer;
- (void)dispatchTransferCreateIfNeeded;
- (void)removeTransfer:(NSString*)transferId;
- (nullable LynxView*)lynxView;

@end

@implementation LynxUITransfer

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("transfer-view")
#else
LYNX_REGISTER_UI("transfer-view")
#endif

LYNX_PROP_SETTER("transfer-id", setTransferId, NSString*) {
  NSString* nextTransferId = requestReset ? nil : value;
  if ((_transferId == nextTransferId) || [_transferId isEqualToString:nextTransferId]) {
    return;
  }
  NSString* previousTransferId = _transferId;
  _transferId = [nextTransferId copy];
  if (!self.nodeReady) {
    return;
  }
  if (previousTransferId.length > 0) {
    [self removeTransfer:previousTransferId];
  }
  [self dispatchTransferCreateIfNeeded];
}

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

- (void)onNodeReady {
  [super onNodeReady];
  self.nodeReady = YES;
  [self dispatchTransferCreateIfNeeded];
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  LynxView* lynxView = [self lynxView];
  if (lynxView != nil && self.attachedTransferId.length > 0 && self.wrapperView != nil) {
    [lynxView dispatchTransferDatasetUpdate:self.wrapperView dataset:self.dataset];
  }
}

- (void)onNodeRemoved {
  [super onNodeRemoved];
  [self cleanupTransfer];
}

- (void)dealloc {
  [self cleanupTransfer];
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

- (void)cleanupTransfer {
  if (self.attachedTransferId.length > 0) {
    [self removeTransfer:self.attachedTransferId];
  }
  self.attachedTransferId = nil;
}

- (void)dispatchTransferCreateIfNeeded {
  if (!self.nodeReady || self.transferId.length == 0 || self.wrapperView == nil ||
      [self.attachedTransferId isEqualToString:self.transferId]) {
    return;
  }

  LynxView* lynxView = [self lynxView];
  if (lynxView == nil) {
    self.attachedTransferId = self.transferId;
    return;
  }

  [lynxView dispatchTransferCreate:self.transferId view:self.wrapperView dataset:self.dataset];
  self.attachedTransferId = self.transferId;
}

- (void)removeTransfer:(NSString*)transferId {
  LynxView* lynxView = [self lynxView];
  if (lynxView != nil && self.wrapperView != nil) {
    [lynxView dispatchTransferRemove:transferId view:self.wrapperView];
  } else {
    [self.wrapperView removeFromSuperview];
  }
  self.attachedTransferId = nil;
}

- (nullable LynxView*)lynxView {
  UIView* rootView = self.context.rootView;
  return [rootView isKindOfClass:[LynxView class]] ? (LynxView*)rootView : nil;
}

@end
