// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxConverter+NSShadow.h"
#import "LynxLog.h"

@implementation LynxConverter (NSShadow)
+ (NSShadow *)toNSShadow:(NSArray<LynxBoxShadow *> *)shadowArr {
  if (!shadowArr || [shadowArr count] <= 0) {
    return nil;
  }
  // lynx only support one shadow.
  LynxBoxShadow *boxShadow = shadowArr[0];
  NSShadow *shadow = [[NSShadow alloc] init];
  shadow.shadowOffset = CGSizeMake(boxShadow.offsetX, boxShadow.offsetY);
  shadow.shadowBlurRadius = boxShadow.blurRadius;
  shadow.shadowColor = boxShadow.shadowColor;
  return shadow;
}

@end
