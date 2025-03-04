// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxImageProcessor.h"

NS_ASSUME_NONNULL_BEGIN
/**
 Scale image to fit into the size with padding and border with contentMode
 */
@interface LynxFatImageProcessor : NSObject <LynxImageProcessor>

- (instancetype)initWithSize:(CGSize)size
                     padding:(UIEdgeInsets)padding
                      border:(UIEdgeInsets)border
                 contentMode:(UIViewContentMode)contentMode;

@end

NS_ASSUME_NONNULL_END
