// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "lynx/core/base/thread/atomic_lifecycle.h"

#include <thread>

#include "base/include/log/logging.h"

namespace {
constexpr int32_t STATE_FREE = 0;
constexpr int32_t STATE_LOCKED = 1;
constexpr int32_t STATE_TERMINATED = 2;
}  // namespace

namespace lynx {
namespace base {

bool AtomicLifecycle::TryLock(AtomicLifecycle* ptr) {
  if (ptr == nullptr) {
    return false;
  }

  int32_t previous = STATE_FREE;
  bool locked = ptr->state_.compare_exchange_strong(previous, STATE_LOCKED) ||
                (previous == STATE_LOCKED);

  if (!locked) {
    LOGW("AtomicLifecycle TryLock after TryTerminate");
    return false;
  }

  ++ptr->lock_count_;
  return true;
}

bool AtomicLifecycle::TryFree(AtomicLifecycle* ptr) {
  if (ptr == nullptr) {
    return false;
  }

  int32_t lock_count = --ptr->lock_count_;
  if (lock_count > 0) {
    return false;
  } else if (lock_count < 0) {
    LOGW("AtomicLifecycle TryFree more than TryLock");
  }
  ptr->state_ = STATE_FREE;
  return true;
}

bool AtomicLifecycle::TryTerminate(AtomicLifecycle* ptr) {
  if (ptr == nullptr) {
    return false;
  }

  int32_t previous = STATE_FREE;
  return ptr->state_.compare_exchange_strong(previous, STATE_TERMINATED);
}

}  // namespace base
}  // namespace lynx
