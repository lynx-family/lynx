// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxNestedScrollHelper.h>
#import <Lynx/LynxUI.h>
#import <Lynx/UIScrollView+Lynx.h>

@interface LynxNestedScrollHelper ()
@property(nonatomic, weak) LynxEventHandler *eventHandler;
@property(nonatomic, strong) NSMutableArray<LynxUI *> *scrollTargetChain;
@end

@implementation LynxNestedScrollHelper

- (instancetype)initWithEventHandler:(LynxEventHandler *)eventHandler {
  if (self = [super init]) {
    _eventHandler = eventHandler;
    _scrollTargetChain = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)resetScrollTargetChain {
  for (LynxUI *ui in _scrollTargetChain) {
    ui.parentScroll = nil;
    ui.childScroll = nil;
  }
  [_scrollTargetChain removeAllObjects];
}

- (void)generateScrollTargetChain {
  [self resetScrollTargetChain];
  id<LynxEventTarget> touchTarget = _eventHandler.touchTarget;
  if (touchTarget == nil) {
    return;
  }
  id<LynxEventTarget> currentTarget = touchTarget;
  while (currentTarget) {
    if ([currentTarget isKindOfClass:[LynxUI class]] &&
        [(LynxUI *)currentTarget isScrollContainer]) {
      if (_scrollTargetChain.count != 0) {
        _scrollTargetChain.lastObject.parentScroll = (LynxUI *)currentTarget;
        ((LynxUI *)currentTarget).childScroll = _scrollTargetChain.lastObject.parentScroll;
      }
      [_scrollTargetChain addObject:(LynxUI *)currentTarget];
    }
    currentTarget = currentTarget.parentTarget;
  }
}

@end
