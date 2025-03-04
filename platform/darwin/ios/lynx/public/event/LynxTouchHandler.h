// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
#import "LynxEventEmitter.h"
#import "LynxUIOwner.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxEventHandler;

@interface LynxTouchHandler : UIGestureRecognizer

@property(nonatomic, weak) _Nullable id<LynxEventTarget> target;
@property(nonatomic, weak) _Nullable id<LynxEventTarget> preTarget;

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler;

- (void)onGestureRecognized;

@end

NS_ASSUME_NONNULL_END
