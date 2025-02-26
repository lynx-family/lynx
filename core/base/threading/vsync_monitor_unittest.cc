// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/vsync_monitor.h"

#include <algorithm>

#include "base/include/fml/thread.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

namespace {

constexpr int64_t kFrameDuration = 16;  // ms

constexpr int64_t kLoop = 10;

}  // namespace

class TestVSyncMonitor : public VSyncMonitor {
 public:
  TestVSyncMonitor() = default;
  ~TestVSyncMonitor() override = default;

  void RequestVSync() override {}

  void TriggerVsync() {
    OnVSync(current_, current_ + kFrameDuration);
    current_ += kFrameDuration;
  }

 private:
  int64_t current_ = kFrameDuration;
};

class VsyncMonitorTest : public ::testing::Test {
 protected:
  VsyncMonitorTest() = default;
  ~VsyncMonitorTest() override = default;

  static void SetUpTestSuite() { base::UIThread::Init(); }
};

TEST_F(VsyncMonitorTest, AsyncRequestVSync) {
  auto monitor = std::make_shared<TestVSyncMonitor>();
  monitor->BindToCurrentThread();

  int64_t start_time = 0;
  int64_t target_time = 0;

  for (int64_t i = 1; i <= kLoop; ++i) {
    monitor->AsyncRequestVSync(
        [&start_time, &target_time](int64_t frame_start_time,
                                    int64_t frame_target_time) {
          start_time = frame_start_time;
          target_time = frame_target_time;
        });
    monitor->TriggerVsync();
    ASSERT_EQ(start_time, i * kFrameDuration);
    ASSERT_EQ(target_time, (i + 1) * kFrameDuration);
  }
}

TEST_F(VsyncMonitorTest, AsyncRequestVSyncDuplicately) {
  auto monitor = std::make_shared<TestVSyncMonitor>();
  monitor->BindToCurrentThread();

  int64_t start_time = 0;
  int64_t target_time = 0;

  monitor->AsyncRequestVSync(
      [&start_time, &target_time](int64_t frame_start_time,
                                  int64_t frame_target_time) {
        start_time = frame_start_time;
        target_time = frame_target_time;
      });

  // in one vsync duration, AsyncRequestVSync Duplicately will not be dropped
  monitor->AsyncRequestVSync(
      [&start_time, &target_time](int64_t frame_start_time,
                                  int64_t frame_target_time) {
        start_time = 0;
        target_time = 0;
      });

  monitor->TriggerVsync();
  ASSERT_EQ(start_time, kFrameDuration);
  ASSERT_EQ(target_time, start_time + kFrameDuration);
}

TEST_F(VsyncMonitorTest, AsyncRequestVSyncWithId) {
  auto monitor = std::make_shared<TestVSyncMonitor>();
  monitor->BindToCurrentThread();

  std::vector<int64_t> ids;
  int64_t start_time = 0;
  int64_t target_time = 0;

  for (int64_t i = 0; i < kLoop; ++i) {
    monitor->ScheduleVSyncSecondaryCallback(
        static_cast<uintptr_t>(i),
        [&start_time, &target_time, &ids, i](int64_t frame_start_time,
                                             int64_t frame_target_time) {
          start_time = frame_start_time;
          target_time = frame_target_time;
          ids.emplace_back(i);
        });
  }

  monitor->TriggerVsync();
  ASSERT_EQ(start_time, kFrameDuration);
  ASSERT_EQ(target_time, start_time + kFrameDuration);

  // the callbacks called as the order in a std::unordered_map,
  // not the ascending order.
  std::sort(ids.begin(), ids.end());
  for (int64_t i = 0; i < kLoop; ++i) {
    ASSERT_EQ(ids[static_cast<size_t>(i)], i);
  }
}

TEST_F(VsyncMonitorTest, AsyncRequestVSyncWithDuplicateId) {
  auto monitor = std::make_shared<TestVSyncMonitor>();
  monitor->BindToCurrentThread();

  int64_t start_time = 0;
  int64_t target_time = 0;

  monitor->ScheduleVSyncSecondaryCallback(
      1, [&start_time, &target_time](int64_t frame_start_time,
                                     int64_t frame_target_time) {
        start_time = frame_start_time;
        target_time = frame_target_time;
      });

  // in one vsync duration, AsyncRequestVSync with duplicate id will not be
  // dropped
  monitor->ScheduleVSyncSecondaryCallback(
      1, [&start_time, &target_time](int64_t frame_start_time,
                                     int64_t frame_target_time) {
        start_time = 0;
        target_time = 0;
      });

  monitor->TriggerVsync();
  ASSERT_EQ(start_time, kFrameDuration);
  ASSERT_EQ(target_time, start_time + kFrameDuration);
}

TEST_F(VsyncMonitorTest, AsyncRequestVSyncWithNullCallback) {
  auto monitor = std::make_shared<TestVSyncMonitor>();
  monitor->BindToCurrentThread();

  int64_t start_time = 0;
  int64_t target_time = 0;

  // the null callback will not be dropped
  monitor->ScheduleVSyncSecondaryCallback(1, nullptr);

  monitor->ScheduleVSyncSecondaryCallback(
      1, [&start_time, &target_time](int64_t frame_start_time,
                                     int64_t frame_target_time) {
        start_time = frame_start_time;
        target_time = frame_target_time;
      });

  monitor->TriggerVsync();
  ASSERT_EQ(start_time, kFrameDuration);
  ASSERT_EQ(target_time, start_time + kFrameDuration);
}

TEST_F(VsyncMonitorTest, OnVsyncOnMergedThread) {
  auto monitor = std::make_shared<TestVSyncMonitor>();
  int64_t start_time = 0;
  int64_t target_time = 0;

  fml::Thread owner_thread;
  fml::Thread subsumed_thread;
  auto owner = owner_thread.GetTaskRunner();
  auto subsumed = subsumed_thread.GetTaskRunner();

  std::condition_variable cv_;
  std::mutex mutex_;
  bool has_trigger_finished{false};

  subsumed->PostSyncTask([&monitor, &owner, &cv_, &mutex_,
                          &has_trigger_finished, &start_time, &target_time,
                          owner_id = owner->GetTaskQueueId(),
                          subsumed_id = subsumed->GetTaskQueueId()]() {
    monitor->BindToCurrentThread();
    monitor->AsyncRequestVSync(
        [&start_time, &target_time](int64_t frame_start_time,
                                    int64_t frame_target_time) {
          start_time = frame_start_time;
          target_time = frame_target_time;
        });
    monitor->TriggerVsync();

    // OnVsync on current thread
    ASSERT_EQ(start_time, kFrameDuration);
    ASSERT_EQ(target_time, start_time + kFrameDuration);

    start_time = 0;
    target_time = 0;
    monitor->AsyncRequestVSync(
        [&start_time, &target_time](int64_t frame_start_time,
                                    int64_t frame_target_time) {
          start_time = frame_start_time;
          target_time = frame_target_time;
        });

    fml::MessageLoopTaskQueues::GetInstance()->Merge(owner_id, subsumed_id);
    // Sometimes the owner thread's task execution is even earlier than the task
    // directly scheduled in the subsumed(on current thread). This should be
    // related to the allocation of system thread resources. so we can post a
    // task to wait trigger call, to avoid this situation from happening.
    owner->PostTask([&has_trigger_finished, &cv_, &mutex_]() {
      std::unique_lock<std::mutex> locker(mutex_);
      if (has_trigger_finished) {
        return;
      }
      cv_.wait(locker);
    });
    monitor->TriggerVsync();

    // OnVsync not on current thread
    ASSERT_EQ(start_time, 0);
    ASSERT_EQ(target_time, 0);
    {
      std::unique_lock<std::mutex> locker(mutex_);
      has_trigger_finished = true;
    }
    cv_.notify_all();
  });

  owner->PostSyncTask([owner_id = owner->GetTaskQueueId(),
                       subsumed_id = subsumed->GetTaskQueueId()]() {
    fml::MessageLoopTaskQueues::GetInstance()->Unmerge(owner_id, subsumed_id);
  });

  ASSERT_EQ(start_time, kFrameDuration + kFrameDuration);
  ASSERT_EQ(target_time, start_time + kFrameDuration);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
