// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxColorUtils.h"
#include "core/renderer/css/css_color.h"

@implementation LynxColorUtils

+ (COLOR_CLASS*)convertNSStringToUIColor:(NSString*)value {
  if (value == nil) {
    return NULL;
  }
  std::string str = [value UTF8String];
  lynx::tasm::CSSColor color;
  if (lynx::tasm::CSSColor::Parse(str, color)) {
    return [COLOR_CLASS colorWithRed:((float)color.r_ / 255.0f)
                               green:((float)color.g_ / 255.0f)
                                blue:((float)color.b_ / 255.0f)
                               alpha:((float)color.a_)];
  }
  return NULL;
}

@end
