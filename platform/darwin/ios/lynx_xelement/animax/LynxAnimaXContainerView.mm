// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXImageView.h>
#import <AnimaX/AnimaXView.h>
#import <XElement/LynxAnimaXContainerView.h>

@implementation LynxAnimaXContainerView

- (void)setContentView:(UIView<AnimaXViewProtocol, AnimaXPlayerProtocol> *)view {
  if (_contentView) {
    return;
  }
  if (![view isKindOfClass:AnimaXView.class] && ![view isKindOfClass:AnimaXImageView.class]) {
    return;
  }
  _contentView = view;
  view.translatesAutoresizingMaskIntoConstraints = YES;
  [self addSubview:view];
}

- (void)setPadding:(UIEdgeInsets)padding {
  if (!UIEdgeInsetsEqualToEdgeInsets(_padding, padding)) {
    _padding = padding;
    [self setNeedsLayout];
  }
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (!_contentView) return;
  CGRect contentFrame = UIEdgeInsetsInsetRect(self.bounds, _padding);
  _contentView.frame = contentFrame;
}

@end
