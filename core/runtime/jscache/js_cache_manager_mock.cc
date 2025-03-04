// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_manager.h"

namespace lynx {
namespace piper {
namespace cache {
std::string JsCacheManager::GetPlatformCacheDir() { return "./"; }
}  // namespace cache
}  // namespace piper
}  // namespace lynx
