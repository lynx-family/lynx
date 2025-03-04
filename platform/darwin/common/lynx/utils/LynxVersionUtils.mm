// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxVersionUtils.h"
#include "base/include/string/string_utils.h"
#include "core/template_bundle/template_codec/version.h"

@implementation LynxVersionUtils

// This function takes two strings as parameters, converts them into the type 'Version,' and compare
// the values. It returns '1' if the left string is greater than the right; '0' if they are the same
// version; '-1' if the left is less than the right.
+ (NSInteger)compareLeft:(const NSString*)left withRight:(const NSString*)right {
  lynx::base::Version leftVersion(lynx::base::SafeStringConvert([left UTF8String]));
  lynx::base::Version rightVersion(lynx::base::SafeStringConvert([right UTF8String]));
  if (leftVersion > rightVersion) {
    return 1;
  } else if (leftVersion == rightVersion) {
    return 0;
  } else {
    return -1;
  }
}

@end
