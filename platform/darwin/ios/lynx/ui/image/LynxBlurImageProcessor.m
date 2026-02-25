// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBlurImageProcessor.h>
#import <Lynx/LynxImageBlurUtils.h>

@implementation LynxBlurImageProcessor {
  CGFloat _blurRadius;
  BOOL _useCIGaussianBlur;
}

- (instancetype)initWithBlurRadius:(CGFloat)radius {
  self = [super init];
  if (self) {
    _blurRadius = radius;
    _useCIGaussianBlur = NO;
  }
  return self;
}

- (void)setUseCIGaussianBlur:(BOOL)enable {
  _useCIGaussianBlur = enable;
}

- (UIImage *)processImage:(UIImage *)image {
  if (_blurRadius > 0) {
    if (_useCIGaussianBlur) {
      return [LynxImageBlurUtils ciGaussianBlurImage:image withRadius:_blurRadius];
    }
    return [LynxImageBlurUtils blurImage:image withRadius:_blurRadius];
  }
  return image;
}

- (NSString *)cacheKey {
  return [NSString stringWithFormat:@"BlurImageProcessor_%f", _blurRadius];
}

@end
