// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_SERVICES_TRAIL_TRAIL_SETTING_KEYS_H_
#define SERVICE_API_SERVICES_TRAIL_TRAIL_SETTING_KEYS_H_

#include <service_api/service_api_utils.h>

#include "./trail_setting_key.h"

namespace lynx {
namespace service {
namespace trail_service {

// Examples:
// LYNX_SETTING_KEY(ENABLE_XXX, bool, "xxx", true);
// LYNX_SETTING_KEY(ENABLE_YYY, std::string, "yyy", "clay");
// LYNX_SETTING_KEY(ENABLE_ZZZ, std::string, "zzz");

}  // namespace trail_service
}  // namespace service
}  // namespace lynx

#endif  // SERVICE_API_SERVICES_TRAIL_TRAIL_SETTING_KEYS_H_
