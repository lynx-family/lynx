// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/harmony/js_cache_manager_harmony.h"

#include "core/runtime/jscache/js_cache_manager.h"

namespace lynx {
namespace piper {
namespace cache {

void JsCacheManagerHarmony::SetCacheDir(const std::string& cache_dir) {
  auto& dir = GetCacheDir();
  dir = cache_dir;
}

std::string& JsCacheManagerHarmony::GetCacheDir() {
  static std::string s_cache_dir("");
  return s_cache_dir;
}

std::string JsCacheManager::GetPlatformCacheDir() {
  return JsCacheManagerHarmony::GetCacheDir();
}

}  // namespace cache
}  // namespace piper
}  // namespace lynx
