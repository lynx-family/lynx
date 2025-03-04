// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBlurImageProcessor.h"
#import "LynxImageBlurUtils.h"

@implementation LynxBlurImageProcessor {
  CGFloat _blurRadius;
}

- (instancetype)initWithBlurRadius:(CGFloat)radius {
  self = [super init];
  if (self) {
    _blurRadius = radius;
  }
  return self;
}

- (UIImage *)processImage:(UIImage *)image {
  if (_blurRadius > 0) {
    return [LynxImageBlurUtils blurImage:image withRadius:_blurRadius];
  }
  return image;
}

- (NSString *)cacheKey {
  return [NSString stringWithFormat:@"BlurImageProcessor_%f", _blurRadius];
}

@end
