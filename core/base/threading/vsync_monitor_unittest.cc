// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/vsync_monitor.h"

#include <algorithm>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/thread.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
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

class ExternalVSyncSource : public VSyncMonitorPlatformImpl {
 public:
  void RequestVSync(Callback callback) override {
    ++requests;
    pending = std::move(callback);
  }

  void Tick(int64_t start, int64_t target) {
    auto callback = std::move(pending);
    ASSERT_TRUE(callback);
    callback(start, target);
  }

  int requests = 0;
  Callback pending;
};

TEST_F(VsyncMonitorTest, ExternalSourceCoalescesPrimaryAndSecondaryRequests) {
  auto source = std::make_shared<ExternalVSyncSource>();
  auto monitor = std::make_shared<VSyncMonitor>(source);
  monitor->BindToCurrentThread();
  int callbacks = 0;
  monitor->AsyncRequestVSync([&](int64_t start, int64_t target) {
    EXPECT_EQ(start, 100);
    EXPECT_EQ(target, 200);
    ++callbacks;
  });
  monitor->ScheduleVSyncSecondaryCallback(
      1, [&](int64_t, int64_t) { ++callbacks; });
  EXPECT_EQ(source->requests, 1);
  EXPECT_EQ(callbacks, 0);
  source->Tick(100, 200);
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(callbacks, 2);
}

TEST_F(VsyncMonitorTest, ExternalSourceDefersReentrantRequests) {
  auto source = std::make_shared<ExternalVSyncSource>();
  auto monitor = std::make_shared<VSyncMonitor>(source);
  monitor->BindToCurrentThread();
  int callbacks = 0;
  monitor->AsyncRequestVSync([&](int64_t, int64_t) {
    ++callbacks;
    monitor->ScheduleVSyncSecondaryCallback(
        2, [&](int64_t, int64_t) { ++callbacks; });
  });
  EXPECT_EQ(source->requests, 1);
  source->Tick(100, 200);
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(callbacks, 1);
  EXPECT_EQ(source->requests, 2);
  source->Tick(200, 300);
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(callbacks, 2);
}

TEST_F(VsyncMonitorTest, ExternalSignalReturnsToBoundRunner) {
  fml::Thread thread("external-vsync-test");
  auto source = std::make_shared<ExternalVSyncSource>();
  auto monitor = std::make_shared<VSyncMonitor>(source);
  int callbacks = 0;
  thread.GetTaskRunner()->PostSyncTask([&] {
    monitor->BindToCurrentThread();
    monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t start, int64_t) {
      EXPECT_TRUE(thread.GetTaskRunner()->RunsTasksOnCurrentThread());
      EXPECT_EQ(start, 100);
      ++callbacks;
    });
  });
  EXPECT_EQ(callbacks, 0);
  source->Tick(100, 200);
  thread.GetTaskRunner()->PostSyncTask([] {});
  EXPECT_EQ(callbacks, 1);
}

TEST_F(VsyncMonitorTest, SynchronousHostDoesNotInvokeConsumersInline) {
  class SynchronousSource : public VSyncMonitorPlatformImpl {
   public:
    void RequestVSync(Callback callback) override { callback(100, 200); }
  };
  auto monitor =
      std::make_shared<VSyncMonitor>(std::make_shared<SynchronousSource>());
  monitor->BindToCurrentThread();
  bool called = false;
  monitor->AsyncRequestVSync([&](int64_t, int64_t) { called = true; });
  EXPECT_FALSE(called);
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_TRUE(called);
}

TEST_F(VsyncMonitorTest, StopDiscardsAnAlreadyQueuedExternalSignal) {
  fml::Thread thread("external-vsync-stop-test");
  auto source = std::make_shared<ExternalVSyncSource>();
  auto monitor = std::make_shared<VSyncMonitor>(source);
  int callbacks = 0;
  thread.GetTaskRunner()->PostSyncTask([&] {
    monitor->BindToCurrentThread();
    monitor->AsyncRequestVSync([&](int64_t, int64_t) { ++callbacks; });
    // Queue delivery while the owner is still executing this task.
    std::thread host([&] { source->Tick(100, 200); });
    host.join();
    monitor->StopVSync();
  });
  thread.GetTaskRunner()->PostSyncTask([] {});
  EXPECT_EQ(callbacks, 0);
}

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
}  // namespace base
}  // namespace lynx
