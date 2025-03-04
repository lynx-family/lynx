// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Accelerate/Accelerate.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxImageBlurUtils : NSObject

/**
 Blur input image with given radius
 @param inputImage  image ready to blur
 @param radius      radius (in point) to blur
 */
+ (UIImage*)blurImage:(UIImage*)inputImage withRadius:(CGFloat)radius;

@end

NS_ASSUME_NONNULL_END
