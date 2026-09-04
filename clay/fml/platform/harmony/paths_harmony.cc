// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/platform/harmony/paths_harmony.h"

#include <string>
#include <utility>

#include "base/include/no_destructor.h"
#include "clay/fml/file.h"

namespace fml {
namespace paths {

std::pair<bool, std::string> GetExecutablePath() { return {false, ""}; }

namespace {

std::string& GetCachesPath() {
  static lynx::base::NoDestructor<std::string> caches_path;
  return *caches_path;
}

}  // namespace

void InitializeHarmonyCachesPath(std::string caches_path) {
  GetCachesPath() = std::move(caches_path);
}

fml::UniqueFD GetCachesDirectory() {
  // If the caches path is not initialized, the FD will be invalid and caching
  // will be disabled throughout the system.
  return OpenDirectory(GetCachesPath().c_str(), false,
                       fml::FilePermission::kRead);
}

}  // namespace paths
}  // namespace fml
