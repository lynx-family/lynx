// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_SYNCHRONIZATION_COUNT_DOWN_LATCH_H_
#define BASE_INCLUDE_FML_SYNCHRONIZATION_COUNT_DOWN_LATCH_H_

#include <atomic>

#include "base/include/fml/macros.h"
#include "base/include/fml/synchronization/waitable_event.h"

namespace lynx {
namespace fml {

class CountDownLatch {
 public:
  explicit CountDownLatch(size_t count);

  ~CountDownLatch();

  void Wait();

  void CountDown();

 private:
  std::atomic_size_t count_;
  ManualResetWaitableEvent waitable_event_;

  BASE_DISALLOW_COPY_AND_ASSIGN(CountDownLatch);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::CountDownLatch;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_SYNCHRONIZATION_COUNT_DOWN_LATCH_H_
