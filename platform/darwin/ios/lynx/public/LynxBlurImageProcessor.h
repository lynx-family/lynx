// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxImageProcessor.h"

NS_ASSUME_NONNULL_BEGIN

/**
 Use to process image into blurred image with given radius
 */
@interface LynxBlurImageProcessor : NSObject <LynxImageProcessor>

- (instancetype)initWithBlurRadius:(CGFloat)radius;

@end

NS_ASSUME_NONNULL_END
