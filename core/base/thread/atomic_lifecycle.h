// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREAD_ATOMIC_LIFECYCLE_H_
#define CORE_BASE_THREAD_ATOMIC_LIFECYCLE_H_

#include <atomic>

namespace lynx {
namespace base {

class AtomicLifecycle {
 public:
  static bool TryLock(AtomicLifecycle*);
  static bool TryFree(AtomicLifecycle*);
  static bool TryTerminate(AtomicLifecycle*);

  AtomicLifecycle() = default;
  ~AtomicLifecycle() = default;

 private:
  std::atomic<std::int32_t> state_ = 0;
  std::atomic<std::int32_t> lock_count_ = 0;
};

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREAD_ATOMIC_LIFECYCLE_H_
