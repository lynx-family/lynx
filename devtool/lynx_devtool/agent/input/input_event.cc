// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/input/input_event.h"

#include <atomic>

namespace lynx {
namespace input {

int32_t GenerateSyntheticPointerId() {
  // Android reserves pointer ids to five bits. Keep zero available for
  // legacy single-pointer adapters and cycle through the common range.
  constexpr int32_t kFirstSyntheticPointerId = 1;
  constexpr int32_t kLastSyntheticPointerId = 31;
  static std::atomic<int32_t> next_id{kFirstSyntheticPointerId};
  int32_t id = next_id.load();
  while (true) {
    const int32_t following_id =
        id == kLastSyntheticPointerId ? kFirstSyntheticPointerId : id + 1;
    if (next_id.compare_exchange_weak(id, following_id)) {
      return id;
    }
  }
}

}  // namespace input
}  // namespace lynx
