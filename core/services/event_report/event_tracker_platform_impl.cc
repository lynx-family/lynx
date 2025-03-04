// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace tasm {
namespace report {

void EventTrackerPlatformImpl::OnEvent(int32_t instance_id,
                                       MoveOnlyEvent&& event) {
  // Do nothing by deafult.
}

void EventTrackerPlatformImpl::OnEvents(int32_t instance_id,
                                        std::vector<MoveOnlyEvent> stack) {
  // Do nothing by deafult.
  // TODO(limeng.amer): Add Darwin、Android、Win platform layer implementation.
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {
  // Do nothing by deafult.
  // TODO(limeng.amer): Add Darwin、Android、Win platform layer implementation.
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {}
void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const float value) {}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
