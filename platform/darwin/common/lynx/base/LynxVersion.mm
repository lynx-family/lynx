// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxVersion.h>

#include "core/renderer/tasm/config.h"

@implementation LynxVersion

+ (NSString*)versionString {
#ifndef Lynx_POD_VERSION
#define Lynx_POD_VERSION nil
#endif
  NSString* podVersion = Lynx_POD_VERSION;
  if (podVersion) {
    return [podVersion substringFromIndex:5];
  }
  return [NSString stringWithUTF8String:lynx::tasm::Config::GetCurrentLynxVersion().c_str()];
}
@end
