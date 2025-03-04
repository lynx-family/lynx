// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_INCLUDE_FML_SYNCHRONIZATION_SHARED_MUTEX_STD_H_
#define BASE_INCLUDE_FML_SYNCHRONIZATION_SHARED_MUTEX_STD_H_

#include <shared_mutex>  // NOLINT

#include "base/include/fml/synchronization/shared_mutex.h"

namespace lynx {
namespace fml {

class SharedMutexStd : public SharedMutex {
 public:
  virtual void Lock();
  virtual void LockShared();
  virtual void Unlock();
  virtual void UnlockShared();

 private:
  friend SharedMutex* SharedMutex::Create();
  SharedMutexStd() = default;

  std::shared_timed_mutex mutex_;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_SYNCHRONIZATION_SHARED_MUTEX_STD_H_
