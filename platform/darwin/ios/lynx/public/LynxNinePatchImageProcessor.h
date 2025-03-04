// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxImageProcessor.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxNinePatchImageProcessor : NSObject <LynxImageProcessor>

- (instancetype)initWithCapInsets:(UIEdgeInsets)capInsets;
- (instancetype)initWithCapInsets:(UIEdgeInsets)capInsets capInsetsScale:(CGFloat)capInsetsScale;

@end

NS_ASSUME_NONNULL_END
