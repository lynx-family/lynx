// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_ANDROID_PATHS_ANDROID_H_
#define CLAY_FML_PLATFORM_ANDROID_PATHS_ANDROID_H_

#include <string>

#include "base/include/fml/macros.h"
#include "clay/fml/paths.h"

namespace fml {
namespace paths {

void InitializeAndroidCachesPath(std::string caches_path);

}  // namespace paths
}  // namespace fml

#endif  // CLAY_FML_PLATFORM_ANDROID_PATHS_ANDROID_H_
