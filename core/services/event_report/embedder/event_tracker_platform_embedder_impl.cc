// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/services/event_report/event_tracker_platform_impl.h"
#include "platform/embedder/core/performance/lynx_event_reporter.h"

namespace lynx {
namespace tasm {
namespace report {

void EventTrackerPlatformImpl::OnEvent(int32_t instance_id,
                                       MoveOnlyEvent&& event) {
  // TODO(zhangpengcheng): implement onEvent for embedder
}

void EventTrackerPlatformImpl::OnEvents(int32_t instance_id,
                                        std::vector<MoveOnlyEvent> stack) {
  // TODO(zhangpengcheng): implement onEvent for embedder
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {
  embedder::LynxEventReporter::UpdateGenericInfo(std::move(generic_info),
                                                 instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float> generic_info) {
  embedder::LynxEventReporter::UpdateGenericInfo(std::move(generic_info),
                                                 instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {
  embedder::LynxEventReporter::UpdateGenericInfo(key, value, instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {
  embedder::LynxEventReporter::UpdateGenericInfo(key, value, instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const float value) {
  embedder::LynxEventReporter::UpdateGenericInfo(key, value, instance_id);
}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  embedder::LynxEventReporter::ClearCache(instance_id);
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
