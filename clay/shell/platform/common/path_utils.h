// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_COMMON_PATH_UTILS_H_
#define CLAY_SHELL_PLATFORM_COMMON_PATH_UTILS_H_

#include <filesystem>

namespace clay {

// Returns the path of the directory containing this executable, or an empty
// path if the directory cannot be found.
std::filesystem::path GetExecutableDirectory();

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_COMMON_PATH_UTILS_H_
