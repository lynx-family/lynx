// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_COMMON_TRAIL_SETTINGS_H_
#define CLAY_COMMON_TRAIL_SETTINGS_H_

#include <string>

#include "service_api/services/trail/trail_service.h"

namespace clay {
namespace setting {

// Examples:
// LYNX_SETTING_KEY(CLAY_DEMO_TEXT, std::string, "clay_demo_text");
// LYNX_SETTING_KEY(CLAY_DEMO_TEXT_WITH_DEFAULT, std::string,
// "clay_demo_text_1", "demo");
// LYNX_SETTING_KEY(CLAY_ENABLE_FEATURE_A, bool, "clay_enable_feature_a");
// LYNX_SETTING_KEY(CLAY_ENABLE_FEATURE_B, bool, "clay_enable_feature_b", true);

LYNX_SETTING_KEY(CLAY_PRECOMPILE_SKITY_SHADERS, bool,
                 "clay_precompile_skity_shaders");
LYNX_SETTING_KEY(MERGE_CLAY_THREAD_POOL, bool, "merge_clay_thread_pool", true);
LYNX_SETTING_KEY(CLAY_DEFER_VIDEO_PLAYER_ITEM_CREATION, bool,
                 "clay_defer_video_player_item_creation", true);

}  // namespace setting
}  // namespace clay

#endif  // CLAY_COMMON_TRAIL_SETTINGS_H_
