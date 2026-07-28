// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_entity_id_generator.h"

#include <atomic>
#include <cstdint>
#include <limits>

#include "base/include/log/logging.h"

namespace lynx::shell {

base::LynxEntityId GenerateLynxEntityId() {
  static std::atomic<uint32_t> next_id{0};
  const uint32_t id = next_id.fetch_add(1, std::memory_order_relaxed);
  CHECK(id <=
        static_cast<uint32_t>(std::numeric_limits<base::LynxEntityId>::max()));
  return static_cast<base::LynxEntityId>(id);
}

}  // namespace lynx::shell
