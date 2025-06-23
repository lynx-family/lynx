// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_HARMONY_JS_CACHE_MANAGER_HARMONY_H_
#define CORE_RUNTIME_JSCACHE_HARMONY_JS_CACHE_MANAGER_HARMONY_H_

#include <string>

namespace lynx {
namespace piper {
namespace cache {
class JsCacheManager;

class JsCacheManagerHarmony {
 public:
  static void SetCacheDir(const std::string& cache_dir);

 private:
  JsCacheManagerHarmony() = default;
  ~JsCacheManagerHarmony() = default;

  static std::string& GetCacheDir();
  friend class JsCacheManager;
};

}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_HARMONY_JS_CACHE_MANAGER_HARMONY_H_
