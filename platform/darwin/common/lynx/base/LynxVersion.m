// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxVersion.h>

@implementation LynxVersion

+ (NSString*)versionStringFromPodVersion:(NSString*)podVersion {
  NSRange separatorRange = [podVersion rangeOfString:@"_"];
  if (separatorRange.location == NSNotFound || NSMaxRange(separatorRange) >= podVersion.length) {
    return podVersion;
  }
  NSString* appId = [podVersion substringToIndex:separatorRange.location];
  NSCharacterSet* nonDigitSet = [[NSCharacterSet decimalDigitCharacterSet] invertedSet];
  if (appId.length == 0 || [appId rangeOfCharacterFromSet:nonDigitSet].location != NSNotFound) {
    return podVersion;
  }
  return [podVersion substringFromIndex:NSMaxRange(separatorRange)];
}

+ (NSString*)versionString {
// source build will define Lynx_POD_VERSION
#ifndef Lynx_POD_VERSION
#define Lynx_POD_VERSION @"9999_1.4.0"
#endif
  return [self versionStringFromPodVersion:Lynx_POD_VERSION];
}
@end
