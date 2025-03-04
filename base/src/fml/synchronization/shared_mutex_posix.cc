// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/include/fml/synchronization/shared_mutex_posix.h"

#include "base/include/fml/macros.h"

namespace lynx {
namespace fml {

SharedMutex* SharedMutex::Create() { return new SharedMutexPosix(); }

SharedMutexPosix::SharedMutexPosix() {
  LYNX_BASE_CHECK(pthread_rwlock_init(&rwlock_, nullptr) == 0);
}

void SharedMutexPosix::Lock() { pthread_rwlock_wrlock(&rwlock_); }

void SharedMutexPosix::LockShared() { pthread_rwlock_rdlock(&rwlock_); }

void SharedMutexPosix::Unlock() { pthread_rwlock_unlock(&rwlock_); }

void SharedMutexPosix::UnlockShared() { pthread_rwlock_unlock(&rwlock_); }

}  // namespace fml
}  // namespace lynx
