// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/config.h"

namespace lynx {
namespace tasm {

const char* Config::Platform() {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "macOS";
#else
  return "unknown";
#endif
}

}  // namespace tasm
}  // namespace lynx
