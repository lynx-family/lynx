// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_HARMONY_LYNX_TRAIL_HUB_IMPL_HARMONY_H_
#define CORE_RENDERER_UTILS_HARMONY_LYNX_TRAIL_HUB_IMPL_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "base/include/fml/synchronization/shared_mutex.h"
#include "core/renderer/utils/lynx_trail_hub.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxTrailHubImplHarmony : public LynxTrailHub::TrailImpl {
 public:
  LynxTrailHubImplHarmony() = default;
  ~LynxTrailHubImplHarmony() override = default;

  std::optional<std::string> GetStringForTrailKey(
      const std::string& key) override;

  static napi_value Init(napi_env env, napi_value exports);

  class ConcurrentTrailMap {
   public:
    void SetMap(std::unordered_map<std::string, std::string> map);
    std::optional<std::string> GetValue(const std::string& key);

   private:
    std::unordered_map<std::string, std::string> map_{};
    // TODO(zhoupeng.z): After TrailHub is independent for LynxEnv, this mutex
    // will be removed.
    std::unique_ptr<fml::SharedMutex> mutex_{fml::SharedMutex::Create()};
  };
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_HARMONY_LYNX_TRAIL_HUB_IMPL_HARMONY_H_
