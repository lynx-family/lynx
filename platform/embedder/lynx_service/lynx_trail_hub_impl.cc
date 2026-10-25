// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>

#include "core/renderer/utils/lynx_trail_hub.h"
#include "platform/embedder/public/capi/lynx_service_center_capi.h"
#include "platform/embedder/public/lynx_trail_service.h"

namespace lynx {
namespace tasm {
namespace {
class LynxTrailHubImplEmbedder : public LynxTrailHub::TrailImpl {
 public:
  std::optional<std::string> GetStringForTrailKey(
      const std::string& key) override;
};
}  // namespace

std::optional<std::string> LynxTrailHubImplEmbedder::GetStringForTrailKey(
    const std::string& key) {
  lynx_trail_service_t* trail_service =
      reinterpret_cast<lynx_trail_service_t*>(lynx_service_get_service(
          lynx_service_get_center_instance(), kServiceTypeTrail));
  if (!trail_service) {
    return std::nullopt;
  }

  return lynx::pub::GetTrailStringValue(trail_service, key);
}

std::unique_ptr<LynxTrailHub::TrailImpl> LynxTrailHub::TrailImpl::Create() {
  return std::make_unique<LynxTrailHubImplEmbedder>();
}

}  // namespace tasm
}  // namespace lynx
