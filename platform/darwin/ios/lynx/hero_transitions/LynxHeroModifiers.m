// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxHeroModifiers.h"

@implementation LynxHeroModifiers

- (instancetype)init {
  self = [super init];
  if (self) {
    _transform = CATransform3DIdentity;
  }
  return self;
}

- (instancetype)rotateX:(CGFloat)x y:(CGFloat)y z:(CGFloat)z {
  self.transform = CATransform3DRotate(self.transform, x, 1, 0, 0);
  self.transform = CATransform3DRotate(self.transform, y, 0, 1, 0);
  self.transform = CATransform3DRotate(self.transform, z, 0, 0, 1);
  return self;
}

- (instancetype)translateX:(CGFloat)x y:(CGFloat)y z:(CGFloat)z {
  self.transform = CATransform3DTranslate(self.transform, x, y, z);
  return self;
}

- (instancetype)scaleX:(CGFloat)x y:(CGFloat)y z:(CGFloat)z {
  self.transform = CATransform3DScale(self.transform, x, y, z);
  return self;
}

@end
