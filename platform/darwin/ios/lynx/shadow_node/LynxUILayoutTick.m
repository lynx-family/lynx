// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUILayoutTick.h"
#import "LynxView.h"

@interface LynxUILayoutTick ()

@property(nonatomic, weak) LynxView* root;

@end

@implementation LynxUILayoutTick

- (instancetype)initWithRoot:(LynxView*)root block:(LynxOnLayoutBlock)block {
  self = [super initWithBlock:block];
  if (self) {
    _root = root;
  }
  return self;
}

- (void)requestLayout {
  [super requestLayout];
  if (_root) {
    if ([NSThread isMainThread]) {
      [_root setNeedsLayout];
    } else {
      __weak LynxUILayoutTick* weakSelf = self;
      dispatch_async(dispatch_get_main_queue(), ^{
        __strong LynxUILayoutTick* strongSelf = weakSelf;
        [strongSelf.root setNeedsLayout];
      });
    }
  }
}

- (void)attach:(LynxView* _Nonnull)root {
  _root = root;
}

@end
