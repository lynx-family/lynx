// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/profiling/sampling_profiler.h"

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/thread.h"
#include "clay/testing/testing.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

using testing::_;
using testing::Invoke;

namespace fml {
namespace {
class MockTaskRunner : public fml::TaskRunner {
 public:
  inline static RefPtr<MockTaskRunner> Create() {
    return AdoptRef(new MockTaskRunner());
  }
  MOCK_METHOD1(PostTask, void(lynx::base::closure task));
  MOCK_METHOD2(PostTaskForTime,
               void(lynx::base::closure task, fml::TimePoint target_time));
  MOCK_METHOD2(PostDelayedTask,
               void(lynx::base::closure task, fml::TimeDelta delay));
  MOCK_METHOD0(RunsTasksOnCurrentThread, bool());
  MOCK_METHOD0(GetTaskQueueId, TaskQueueId());

 private:
  MockTaskRunner() : TaskRunner(fml::RefPtr<MessageLoopImpl>()) {}
};
}  // namespace
}  // namespace fml

namespace clay {

TEST(SamplingProfilerTest, DeleteAfterStart) {
  auto thread =
      std::make_unique<fml::Thread>(clay::testing::GetCurrentTestName());
  auto task_runner = fml::MockTaskRunner::Create();
  std::atomic<int> invoke_count = 0;

  EXPECT_CALL(*task_runner, PostDelayedTask(_, _))
      .WillRepeatedly(
          Invoke([&](lynx::base::closure task, fml::TimeDelta delay) {
            invoke_count.fetch_add(1);
            thread->GetTaskRunner()->PostTask(std::move(task));
          }));

  {
    auto profiler = SamplingProfiler(
        "profiler",
        /*profiler_task_runner=*/task_runner, [] { return ProfileSample(); },
        /*num_samples_per_sec=*/1000);
    profiler.Start();
  }
  int invoke_count_at_delete = invoke_count.load();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));  // nyquist
  ASSERT_EQ(invoke_count_at_delete, invoke_count.load());
}

}  // namespace clay
