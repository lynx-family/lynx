// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxVersion.h"

@implementation LynxVersion

+ (NSString*)versionString {
// source build will define Lynx_POD_VERSION
#ifndef Lynx_POD_VERSION
#define Lynx_POD_VERSION @"9999_1.4.0"
#endif
  return [Lynx_POD_VERSION substringFromIndex:5];
}
@end
