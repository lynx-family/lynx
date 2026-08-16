// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_HEAP_SNAPSHOT_H_
#define CORE_RUNTIME_JS_JSI_HEAP_SNAPSHOT_H_

#include <string>

namespace lynx {
namespace runtime {
namespace js {

class HeapSnapshot {
 public:
  HeapSnapshot() = default;
  virtual ~HeapSnapshot() = default;

  HeapSnapshot(const HeapSnapshot&) = delete;
  HeapSnapshot& operator=(const HeapSnapshot&) = delete;

  virtual bool WriteToFile(const std::string& output_path) const = 0;
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_HEAP_SNAPSHOT_H_
