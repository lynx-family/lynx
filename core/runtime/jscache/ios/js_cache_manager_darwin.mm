// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_manager.h"

namespace lynx {
namespace piper {
namespace cache {

std::string JsCacheManager::GetPlatformCacheDir() {
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
  NSString *cache_dir = [paths objectAtIndex:0];
  return [cache_dir UTF8String];
}
}  // namespace cache
}  // namespace piper
}  // namespace lynx
