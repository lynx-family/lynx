// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxHeroModifiers : NSObject

@property(nonatomic, assign) NSTimeInterval duration;
@property(nonatomic, assign) NSTimeInterval delay;
@property(nonatomic, assign) float opacity;
@property(nonatomic) CATransform3D transform;
@property(nonatomic, assign) CGSize size;
@property(nonatomic, assign) CAMediaTimingFunction* timingFunction;

- (instancetype)rotateX:(CGFloat)x y:(CGFloat)y z:(CGFloat)z;
- (instancetype)translateX:(CGFloat)x y:(CGFloat)y z:(CGFloat)z;
- (instancetype)scaleX:(CGFloat)x y:(CGFloat)y z:(CGFloat)z;

@end

NS_ASSUME_NONNULL_END
