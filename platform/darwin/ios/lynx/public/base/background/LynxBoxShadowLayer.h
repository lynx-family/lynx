// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxBoxShadow;
@class LynxUI;

@interface LynxBoxShadowLayer : CALayer
// TODO(renzhongyue): abstract an async display layer and LynxShadowLayer derive from
// it.
@property(nonatomic, strong) UIColor* customShadowColor;
@property(nonatomic, assign) CGFloat customShadowBlur;
@property(nonatomic, assign) CGSize customShadowOffset;
@property(nonatomic, nullable) CGPathRef maskPath;
@property(nonatomic, nullable) CGPathRef customShadowPath;
@property(nonatomic, weak) LynxUI* ui;
@property(nonatomic, assign) BOOL inset;

- (id)initWithUi:(LynxUI* _Nullable)ui;
- (void)invalidate;
@end

NS_ASSUME_NONNULL_END
