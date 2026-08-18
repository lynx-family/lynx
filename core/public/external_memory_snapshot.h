// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_EXTERNAL_MEMORY_SNAPSHOT_H_
#define CORE_PUBLIC_EXTERNAL_MEMORY_SNAPSHOT_H_

#include <cstdint>

namespace lynx {
namespace tasm {

// A current-state estimate of external memory attributed to a page. Garbage is
// the reclaimable subset of the total size.
struct ExternalMemorySnapshot {
  int64_t total_size{0};
  int64_t garbage_size{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_EXTERNAL_MEMORY_SNAPSHOT_H_
