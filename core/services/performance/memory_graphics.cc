// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_graphics.h"

namespace lynx {
namespace tasm {
namespace performance {

bool IsGraphicsMemoryCategory(std::string_view /*category*/) { return false; }

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
