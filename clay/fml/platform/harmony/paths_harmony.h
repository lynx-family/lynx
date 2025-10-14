// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_HARMONY_PATHS_HARMONY_H_
#define CLAY_FML_PLATFORM_HARMONY_PATHS_HARMONY_H_

#include <string>

#include "base/include/fml/macros.h"
#include "clay/fml/paths.h"

namespace fml {
namespace paths {

void InitializeHarmonyCachesPath(std::string caches_path);

}  // namespace paths
}  // namespace fml

#endif  // CLAY_FML_PLATFORM_HARMONY_PATHS_HARMONY_H_
