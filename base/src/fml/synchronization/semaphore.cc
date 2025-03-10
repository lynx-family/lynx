// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/include/fml/synchronization/semaphore.h"

#include "base/include/fml/macros.h"
#include "build/build_config.h"

#if OS_MACOSX
#include <dispatch/dispatch.h>

namespace lynx {
namespace fml {

class PlatformSemaphore {
 public:
  explicit PlatformSemaphore(uint32_t count)
      : sem_(dispatch_semaphore_create(count)), initial_(count) {}

  ~PlatformSemaphore() {
    for (uint32_t i = 0; i < initial_; ++i) {
      Signal();
    }
    if (sem_ != nullptr) {
      dispatch_release(reinterpret_cast<dispatch_object_t>(sem_));
      sem_ = nullptr;
    }
  }

  bool IsValid() const { return sem_ != nullptr; }

  bool Wait() {
    if (sem_ == nullptr) {
      return false;
    }
    return dispatch_semaphore_wait(sem_, DISPATCH_TIME_FOREVER) == 0;
  }

  bool TryWait() {
    if (sem_ == nullptr) {
      return false;
    }

    return dispatch_semaphore_wait(sem_, DISPATCH_TIME_NOW) == 0;
  }

  void Signal() {
    if (sem_ != nullptr) {
      dispatch_semaphore_signal(sem_);
    }
  }

 private:
  dispatch_semaphore_t sem_;
  const uint32_t initial_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformSemaphore);
};

}  // namespace fml
}  // namespace lynx

#elif OS_WIN
#include <windows.h>

namespace lynx {
namespace fml {

class PlatformSemaphore {
 public:
  explicit PlatformSemaphore(uint32_t count)
      : _sem(CreateSemaphore(NULL, count, LONG_MAX, NULL)) {}

  ~PlatformSemaphore() {
    if (_sem != nullptr) {
      CloseHandle(_sem);
      _sem = nullptr;
    }
  }

  bool IsValid() const { return _sem != nullptr; }

  bool Wait() {
    if (_sem == nullptr) {
      return false;
    }

    return WaitForSingleObject(_sem, INFINITE) == WAIT_OBJECT_0;
  }

  bool TryWait() {
    if (_sem == nullptr) {
      return false;
    }

    return WaitForSingleObject(_sem, 0) == WAIT_OBJECT_0;
  }

  void Signal() {
    if (_sem != nullptr) {
      ReleaseSemaphore(_sem, 1, NULL);
    }
  }

 private:
  HANDLE _sem;

  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformSemaphore);
};

}  // namespace fml
}  // namespace lynx

#else
#include <semaphore.h>

#include "base/include/fml/eintr_wrapper.h"

namespace lynx {
namespace fml {

class PlatformSemaphore {
 public:
  explicit PlatformSemaphore(uint32_t count)
      : valid_(::sem_init(&sem_, 0 /* not shared */, count) == 0) {}

  ~PlatformSemaphore() {
    if (valid_) {
      int result = ::sem_destroy(&sem_);
      (void)result;
      // Can only be EINVAL which should not be possible since we checked for
      // validity.
      LYNX_BASE_DCHECK(result == 0);
    }
  }

  bool IsValid() const { return valid_; }

  bool Wait() {
    if (!valid_) {
      return false;
    }

    return FML_HANDLE_EINTR(::sem_wait(&sem_)) == 0;
  }

  bool TryWait() {
    if (!valid_) {
      return false;
    }

    return FML_HANDLE_EINTR(::sem_trywait(&sem_)) == 0;
  }

  void Signal() {
    if (!valid_) {
      return;
    }

    ::sem_post(&sem_);

    return;
  }

 private:
  bool valid_;
  sem_t sem_;

  BASE_DISALLOW_COPY_AND_ASSIGN(PlatformSemaphore);
};

}  // namespace fml
}  // namespace lynx

#endif

namespace lynx {
namespace fml {

Semaphore::Semaphore(uint32_t count) : _impl(new PlatformSemaphore(count)) {}

Semaphore::~Semaphore() = default;

bool Semaphore::IsValid() const { return _impl->IsValid(); }

bool Semaphore::Wait() { return _impl->Wait(); }

bool Semaphore::TryWait() { return _impl->TryWait(); }

void Semaphore::Signal() { return _impl->Signal(); }

}  // namespace fml
}  // namespace lynx
