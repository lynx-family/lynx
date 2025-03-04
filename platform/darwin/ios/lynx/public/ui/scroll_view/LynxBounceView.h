// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIView.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxBounceViewDirection) {
  LynxBounceViewDirectionRight = 0,
  LynxBounceViewDirectionBottom,
  LynxBounceViewDirectionLeft,
  LynxBounceViewDirectionTop,
};

@interface LynxBounceView : LynxUIView

@property(nonatomic, assign) LynxBounceViewDirection direction;
@property(nonatomic, assign) float space;
@property(nonatomic) CGFloat triggerBounceEventDistance;

@end

NS_ASSUME_NONNULL_END
