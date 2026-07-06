// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/js_cache_manager.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "platform/embedder/resource/js_source_loader_desktop.h"

namespace lynx {
namespace runtime {
namespace js {

std::string JSSourceLoaderDesktop::LoadJSSource(const std::string& path) {
  return "";
}

namespace cache {

std::string JsCacheManager::GetPlatformCacheDir() { return "./"; }

}  // namespace cache
}  // namespace js
}  // namespace runtime

namespace tasm {
namespace report {

void EventTrackerPlatformImpl::OnEvent(int32_t instance_id,
                                       MoveOnlyEvent&& event) {}

void EventTrackerPlatformImpl::OnEvents(int32_t instance_id,
                                        std::vector<MoveOnlyEvent> stack) {}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float> generic_info) {}

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
