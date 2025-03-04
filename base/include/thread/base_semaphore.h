// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_THREAD_BASE_SEMAPHORE_H_
#define BASE_INCLUDE_THREAD_BASE_SEMAPHORE_H_

#include <semaphore.h>
#include <time.h>

namespace lynx {
namespace base {
struct Semaphore {
  inline Semaphore(int value) { sem_init(&_sem, 0, value); }

  inline ~Semaphore() { sem_destroy(&_sem); }

  inline void Wait() { sem_wait(&_sem); }

  inline void Wait(long usec) {
#if defined(OS_IOS) || defined(OS_OSX)
    sem_wait(&_sem);
#else
    struct timespec timeout;
    struct timeval tt;
    gettimeofday(&tt, nullptr);

    tt.tv_usec += usec;
    timeout.tv_sec = tt.tv_sec + tt.tv_usec / 1000000;
    timeout.tv_nsec = tt.tv_usec % 1000000 * 1000;

    sem_timedwait(&_sem, &timeout);
#endif
  }

  inline void Post() { sem_post(&_sem); }

 private:
  sem_t _sem;
};
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_THREAD_BASE_SEMAPHORE_H_
