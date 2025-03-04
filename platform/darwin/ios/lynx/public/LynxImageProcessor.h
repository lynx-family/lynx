// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 Describe a image processor when you want to process image after image download.
 @see LynxImageLoader
 */
@protocol LynxImageProcessor <NSObject>

- (UIImage*)processImage:(UIImage*)image;

- (NSString*)cacheKey;

@end

NS_ASSUME_NONNULL_END
