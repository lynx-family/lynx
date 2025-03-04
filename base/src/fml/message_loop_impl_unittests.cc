// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define TIMESENSITIVE(x) TimeSensitiveTest_##x

namespace lynx {

TEST(MessageLoopImpl, TIMESENSITIVE(WakeUpTimersAreSingletons)) {
  auto loop_impl = fml::MessageLoopImpl::Create();

  const auto t1 = fml::TimeDelta::FromMilliseconds(10);
  const auto t2 = fml::TimeDelta::FromMilliseconds(30);

  const auto begin = fml::TimePoint::Now();

  // Register a task scheduled in the future. This schedules a WakeUp call on
  // the MessageLoopImpl with that fml::TimePoint.
  loop_impl->PostTask(
      [&]() {
        auto delta = fml::TimePoint::Now() - begin;
        auto ms = delta.ToMillisecondsF();
        ASSERT_GE(ms, 20);
        ASSERT_LE(ms, 40);

        loop_impl->Terminate();
      },
      begin + t1);

  // Call WakeUp manually to change the WakeUp time further in the future. If
  // the timer is correctly set up to be rearmed instead of a task being
  // scheduled for each WakeUp, the above task will be executed at t2 instead of
  // t1 now.
  loop_impl->WakeUp(begin + t2);

  loop_impl->Run();
}

}  // namespace lynx
