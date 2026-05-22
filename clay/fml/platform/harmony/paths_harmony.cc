// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/platform/harmony/paths_harmony.h"

#include <string>
#include <utility>

#include "clay/fml/file.h"

namespace fml {
namespace paths {

std::pair<bool, std::string> GetExecutablePath() { return {false, ""}; }

static std::string gCachesPath;  // NOLINT

void InitializeHarmonyCachesPath(std::string caches_path) {
  gCachesPath = std::move(caches_path);
}

fml::UniqueFD GetCachesDirectory() {
  // If the caches path is not initialized, the FD will be invalid and caching
  // will be disabled throughout the system.
  return OpenDirectory(gCachesPath.c_str(), false, fml::FilePermission::kRead);
}

}  // namespace paths
}  // namespace fml
