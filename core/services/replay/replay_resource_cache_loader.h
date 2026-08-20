// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_REPLAY_RESOURCE_CACHE_LOADER_H_
#define CORE_SERVICES_REPLAY_REPLAY_RESOURCE_CACHE_LOADER_H_

#include <functional>
#include <string>
#include <vector>

#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace replay {

class ReplayResourceCache;

void LoadRecordScriptsIntoReplayResourceCache(
    ReplayResourceCache* cache, const rapidjson::Document& dom,
    std::function<std::string(const char*)> decode_fn,
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)>
        decompress_fn);

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_REPLAY_RESOURCE_CACHE_LOADER_H_
