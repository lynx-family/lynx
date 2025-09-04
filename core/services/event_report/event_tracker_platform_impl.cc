// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

#include "core/services/event_report/generic_info_storage.h"

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
  // Default impl
  GenericInfoStorage::Instance().UpdateGenericInfo(generic_info, instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float> generic_info) {
  // Default impl
  GenericInfoStorage::Instance().UpdateGenericInfo(generic_info, instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {
  // Default impl
  GenericInfoStorage::Instance().UpdateGenericInfo(key, value, instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {
  // Default impl
  GenericInfoStorage::Instance().UpdateGenericInfo(key, value, instance_id);
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const float value) {
  // Default impl
  GenericInfoStorage::Instance().UpdateGenericInfo(key, value, instance_id);
}

void EventTrackerPlatformImpl::GetAllGenericInfosInReportThread(
    int32_t instance_id,
    const std::function<void(std::unique_ptr<pub::Value> entry)>&
        on_get_generic_infos_cb) {
  // Default impl
  GenericInfoStorage::Instance().GetAllGenericInfosInReportThread(
      instance_id, on_get_generic_infos_cb);
}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  // Default impl
  GenericInfoStorage::Instance().ClearCache(instance_id);
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
