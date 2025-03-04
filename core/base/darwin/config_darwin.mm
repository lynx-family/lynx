// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/base/darwin/config_darwin.h"
#import <Foundation/Foundation.h>
#if OS_IOS
#import "LynxEnv+Internal.h"
#endif

namespace lynx {
namespace tasm {

std::optional<std::string> LynxConfigDarwin::stringFromExternalEnv(const std::string &key) {
#if OS_IOS
  NSString *value =
      [[LynxEnv sharedInstance] _stringFromExternalEnv:[NSString stringWithUTF8String:key.c_str()]];
  if (value) {
    return [value UTF8String];
  }
#endif
  return std::nullopt;
}

}  // namespace tasm
}  // namespace lynx
