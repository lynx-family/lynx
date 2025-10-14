// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/paths.h"

#include <Foundation/Foundation.h>

#include "clay/fml/file.h"

namespace fml {
namespace paths {

std::pair<bool, std::string> GetExecutablePath() {
  @autoreleasepool {
    return {true, [NSBundle mainBundle].executablePath.UTF8String};
  }
}

fml::UniqueFD GetCachesDirectory() {
  @autoreleasepool {
    auto items = [[NSFileManager defaultManager] URLsForDirectory:NSCachesDirectory
                                                        inDomains:NSUserDomainMask];
    if (items.count == 0) {
      return {};
    }

    return OpenDirectory(items[0].fileSystemRepresentation, false, FilePermission::kRead);
  }
}

}  // namespace paths
}  // namespace fml
