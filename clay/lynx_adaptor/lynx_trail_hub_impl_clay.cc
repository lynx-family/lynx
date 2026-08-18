// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/lynx_adaptor/lynx_trail_hub_impl_clay.h"

#include <memory>

#include "platform/embedder/public/capi/lynx_service_center_capi.h"
#include "platform/embedder/public/capi/lynx_trail_service_capi.h"

namespace lynx {
namespace tasm {

std::optional<std::string> LynxTrailHubImplClay::GetStringForTrailKey(
    const std::string& key) {
  lynx_trail_service_t* trail_service =
      reinterpret_cast<lynx_trail_service_t*>(lynx_service_get_service(
          lynx_service_get_center_instance(), kServiceTypeTrail));
  if (!trail_service) {
    return std::nullopt;
  }

  std::optional<std::string> value;
  lynx_trail_service_get_string_value(
      trail_service, key.c_str(),
      [](const char* raw_value, void* user_data) {
        if (raw_value) {
          *static_cast<std::optional<std::string>*>(user_data) = raw_value;
        }
      },
      &value);
  return value;
}

std::unique_ptr<LynxTrailHub::TrailImpl> LynxTrailHub::TrailImpl::Create() {
  return std::make_unique<LynxTrailHubImplClay>();
}

}  // namespace tasm
}  // namespace lynx
