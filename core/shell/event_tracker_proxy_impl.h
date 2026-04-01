// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_EVENT_TRACKER_PROXY_IMPL_H_
#define CORE_SHELL_EVENT_TRACKER_PROXY_IMPL_H_

#include <string>
#include <unordered_map>

#include "core/public/event_tracker_proxy.h"

namespace lynx {
namespace shell {

class EventTrackerProxyImpl : public EventTrackerProxy {
 public:
  ~EventTrackerProxyImpl() override = default;

  void OnEvent(EventBuilder builder) override;

  void UpdateGenericInfo(int32_t instance_id, std::string key,
                         std::string value) override;
  void UpdateGenericInfo(int32_t instance_id, std::string key,
                         float value) override;
  void UpdateGenericInfo(int32_t instance_id, std::string key,
                         int64_t value) override;
  void UpdateGenericInfo(
      int32_t instance_id,
      std::unordered_map<std::string, float>&& prop_map) override;
  void UpdateGenericInfo(
      int32_t instance_id,
      std::unordered_map<std::string, std::string>&& prop_map) override;
  void ClearCache(int32_t instance_id) override;
  void Flush(int32_t instance_id) override;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_EVENT_TRACKER_PROXY_IMPL_H_
