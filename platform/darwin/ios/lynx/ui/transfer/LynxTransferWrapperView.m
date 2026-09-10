// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTransferWrapperView.h"

#import "LynxUITransfer.h"

@interface LynxUITransfer (LynxTransferWrapperView)

- (void)syncHostConstraintsFromWrapper:(LynxTransferWrapperView*)wrapperView;

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

@end
