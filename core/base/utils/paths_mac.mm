
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/utils/paths_mac.h"

#import <Foundation/Foundation.h>

namespace lynx {
namespace common {

std::pair<bool, std::string> GetResourceDirectoryPath() {
  const char *exe_path =
      [[[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/Resources"] UTF8String];
  return {true, std::string(exe_path)};
}

}  // namespace common
}  // namespace lynx
