// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxAnimationTransformRotation : NSObject

@property(nonatomic, assign) CGFloat rotationX;
@property(nonatomic, assign) CGFloat rotationY;
@property(nonatomic, assign) CGFloat rotationZ;

- (BOOL)isEqualToTransformRotation:(LynxAnimationTransformRotation*)transformRotation;

@end

NS_ASSUME_NONNULL_END
