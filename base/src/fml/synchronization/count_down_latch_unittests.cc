// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <chrono>
#include <thread>

#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/thread.h"
#include "build/build_config.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {

TEST(CountDownLatchTest, CanWaitOnZero) {
  CountDownLatch latch(0);
  latch.Wait();
}

TEST(CountDownLatchTest, CanWait) {
  fml::Thread thread("test_thread");
  const size_t count = 100;
  size_t current_count = 0;
  CountDownLatch latch(count);
  auto decrement_latch_on_thread = [runner = thread.GetTaskRunner(), &latch,
                                    &current_count]() {
    runner->PostTask([&latch, &current_count]() {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      current_count++;
      latch.CountDown();
    });
  };
  for (size_t i = 0; i < count; ++i) {
    decrement_latch_on_thread();
  }
  latch.Wait();
  ASSERT_EQ(current_count, count);
}

}  // namespace fml
}  // namespace lynx
