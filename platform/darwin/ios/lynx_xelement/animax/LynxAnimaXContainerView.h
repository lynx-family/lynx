// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXPlayerProtocol.h>
#import <AnimaX/AnimaXViewProtocol.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxAnimaXContainerView : UIView

@property(nonatomic, strong, nullable)
    UIView<AnimaXViewProtocol, AnimaXPlayerProtocol> *contentView;
@property(nonatomic, assign) UIEdgeInsets padding;

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
