// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxConverter.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxBoxShadow : NSObject

@property(nonatomic, strong) UIColor *shadowColor;
@property(nonatomic, assign) CGFloat offsetX;
@property(nonatomic, assign) CGFloat offsetY;
@property(nonatomic, assign) CGFloat blurRadius;
@property(nonatomic, assign) CGFloat spreadRadius;
@property(nonatomic, assign) BOOL inset;
@property(nonatomic, strong) CALayer *layer;

- (BOOL)isEqualToBoxShadow:(LynxBoxShadow *)other;

@end

@class LynxUI;
@interface LynxConverter (LynxBoxShadow)
+ (NSArray<LynxBoxShadow *> *)toLynxBoxShadow:(id)value;
@end

NS_ASSUME_NONNULL_END
