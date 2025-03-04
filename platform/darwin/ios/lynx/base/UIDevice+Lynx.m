// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <sys/utsname.h>
#import "UIDevice+Lynx.h"

@implementation UIDevice (Lynx)

+ (BOOL)lynx_isIPhoneX {
  static BOOL isIPhoneX = NO;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    struct utsname systemInfo;
    uname(&systemInfo);
    NSString* platform = [NSString stringWithCString:systemInfo.machine
                                            encoding:NSUTF8StringEncoding];
    isIPhoneX = [platform isEqualToString:@"iPhone12,1"];
  });
  return isIPhoneX;
}

@end
