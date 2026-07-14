// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_COMMON_TRAIL_SETTINGS_H_
#define CLAY_COMMON_TRAIL_SETTINGS_H_

#include <string>

#include "service_api/services/trail/trail_service.h"

namespace clay {
namespace setting {

LYNX_SETTING_KEY(ENABLE_MEMORY_MONITOR, bool, "enable_memory_monitor");

LYNX_SETTING_KEY(CLAY_PRECOMPILE_SKITY_SHADERS, bool,
                 "clay_precompile_skity_shaders");

}  // namespace setting
}  // namespace clay

#endif  // CLAY_COMMON_TRAIL_SETTINGS_H_
