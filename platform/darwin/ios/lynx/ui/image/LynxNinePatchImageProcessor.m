// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxNinePatchImageProcessor.h"

@implementation LynxNinePatchImageProcessor {
  UIEdgeInsets _capInsets;
  CGFloat _capInsetsScale;
}

- (instancetype)initWithCapInsets:(UIEdgeInsets)capInsets {
  return [self initWithCapInsets:capInsets capInsetsScale:1.0];
}

- (instancetype)initWithCapInsets:(UIEdgeInsets)capInsets capInsetsScale:(CGFloat)capInsetsScale {
  self = [super init];
  if (self) {
    _capInsets = capInsets;
    _capInsetsScale = capInsetsScale;
  }
  return self;
}

- (UIImage *)processImage:(UIImage *)image {
  UIImage *scaledImage = [UIImage imageWithCGImage:[image CGImage]
                                             scale:_capInsetsScale
                                       orientation:image.imageOrientation];
  UIImage *ninePatchImage =
      [scaledImage resizableImageWithCapInsets:UIEdgeInsetsMake(_capInsets.top, _capInsets.left,
                                                                _capInsets.bottom, _capInsets.right)
                                  resizingMode:UIImageResizingModeStretch];
  return ninePatchImage;
}

- (NSString *)cacheKey {
  return [NSString stringWithFormat:@"_NinePatchImage_%@", NSStringFromUIEdgeInsets(_capInsets)];
}

@end
