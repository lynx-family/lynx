// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/src/application_directory/application_directory_impl.h"

#import <Foundation/Foundation.h>

namespace lynx {
namespace base {

std::string GetDefaultApplicationDataDirectory() {
  @autoreleasepool {
    NSString* directory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                               NSUserDomainMask, YES) lastObject];
    const char* path = directory.UTF8String;
    return path ? path : "";
  }
}

}  // namespace base
}  // namespace lynx
