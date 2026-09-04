// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/public/lynx_resource_loader.h"

#if ENABLE_TESTBENCH_REPLAY
#include "core/services/replay/replay_resource_cache.h"
#endif  // ENABLE_TESTBENCH_REPLAY

namespace lynx {
namespace pub {

void LynxResourceLoader::LoadResource(
    const LynxResourceRequest& request,
    base::MoveOnlyClosure<void, LynxResourceResponse&> callback) {
#if ENABLE_TESTBENCH_REPLAY
  auto* cache = replay_cache_ ? replay_cache_.get() : nullptr;
  if (cache != nullptr && !cache->IsEmpty()) {
    LynxResourceResponse cached_response;
    if (cache->ResolveResource(request, &cached_response)) {
      callback(cached_response);
      return;
    }
  }
#endif  // ENABLE_TESTBENCH_REPLAY
  LoadResourceInternal(request, std::move(callback));
}

void LynxResourceLoader::LoadResourcePath(
    const LynxResourceRequest& request,
    base::MoveOnlyClosure<void, LynxPathResponse&> path_callback) {
  LoadResourcePathInternal(request, std::move(path_callback));
}

void LynxResourceLoader::SetReplayResourceCache(
    std::shared_ptr<tasm::replay::ReplayResourceCache> cache) {
  replay_cache_ = std::move(cache);
}
}  // namespace pub
}  // namespace lynx
