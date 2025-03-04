// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/include/fml/synchronization/semaphore.h"

#include <thread>

#include "base/include/fml/thread.h"
#include "base/include/fml/time/time_point.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(SemaphoreTest, SimpleValidity) {
  fml::Semaphore sem(100);
  ASSERT_TRUE(sem.IsValid());
}

TEST(SemaphoreTest, WaitOnZero) {
  fml::Semaphore sem(0);
  ASSERT_FALSE(sem.TryWait());
}

TEST(SemaphoreTest, WaitOnZeroSignalThenWait) {
  fml::Semaphore sem(0);
  ASSERT_FALSE(sem.TryWait());
  std::thread thread([&sem]() { sem.Signal(); });
  thread.join();
  ASSERT_TRUE(sem.TryWait());
  ASSERT_FALSE(sem.TryWait());
}

TEST(SemaphoreTest, IndefiniteWait) {
  auto start = fml::TimePoint::Now();
  constexpr double wait_in_seconds = 0.25;
  fml::Semaphore sem(0);
  ASSERT_TRUE(sem.IsValid());
  fml::Thread signaller("signaller_thread");
  signaller.GetTaskRunner()->PostTaskForTime(
      [&sem]() { sem.Signal(); },
      start + fml::TimeDelta::FromSecondsF(wait_in_seconds));
  ASSERT_TRUE(sem.Wait());
  auto delta = fml::TimePoint::Now() - start;
  ASSERT_GE(delta.ToSecondsF(), wait_in_seconds);
  signaller.Join();
}
