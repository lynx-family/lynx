// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/replay_resource_cache_loader.h"

#include <string>
#include <utility>
#include <vector>

#include "core/services/replay/replay_resource_cache.h"

namespace lynx {
namespace tasm {
namespace replay {

void LoadRecordScriptsIntoReplayResourceCache(
    ReplayResourceCache* cache, const rapidjson::Document& dom,
    std::function<std::string(const char*)> decode_fn,
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>
        decompress_fn) {
  if (cache == nullptr) {
    return;
  }

  if (dom.HasMember("Scripts") && dom["Scripts"].IsObject()) {
    const auto& scripts = dom["Scripts"];
    for (auto it = scripts.MemberBegin(); it != scripts.MemberEnd(); ++it) {
      if (!it->name.IsString() || !it->value.IsString()) {
        continue;
      }
      std::string decoded = decode_fn(it->value.GetString());
      if (decoded.empty()) {
        continue;
      }
      std::vector<uint8_t> compressed_data(decoded.begin(), decoded.end());
      std::vector<uint8_t> data = decompress_fn(compressed_data);
      if (data.empty()) {
        continue;
      }
      cache->AddExternalScript(it->name.GetString(), std::move(data));
    }
  }
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
