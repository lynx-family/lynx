// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_THREAD_PTHREAD_RW_LOCK_GUARD_H_
#define BASE_INCLUDE_THREAD_PTHREAD_RW_LOCK_GUARD_H_

#include <pthread/pthread.h>

#include "base/include/fml/macros.h"

namespace lynx {
namespace base {

class PthreadRWLockGuard {
 public:
  enum class Type : int {
    write = 0,
    read = 1,
  };

  PthreadRWLockGuard(pthread_rwlock_t& lock, Type type) : lock_(lock) {
    if (type == Type::write) {
      pthread_rwlock_wrlock(&lock_);
    } else if (type == Type::read) {
      pthread_rwlock_rdlock(&lock_);
    }
  }

  ~PthreadRWLockGuard() { pthread_rwlock_unlock(&lock_); }

 private:
  pthread_rwlock_t& lock_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PthreadRWLockGuard);
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_THREAD_PTHREAD_RW_LOCK_GUARD_H_
