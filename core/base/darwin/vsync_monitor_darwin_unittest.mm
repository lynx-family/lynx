// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner_delegate.h"
#include "base/include/fml/thread.h"
#include "core/base/threading/vsync_monitor.h"

namespace {

using lynx::base::VSyncMonitor;

class FrameTaskRunnerDelegate : public lynx::fml::TaskRunnerDelegate {
 public:
  explicit FrameTaskRunnerDelegate(const lynx::fml::RefPtr<lynx::fml::TaskRunner> &runner)
      : runner_(runner) {}

  void PostTask(lynx::base::closure task) override {
    if ([NSThread isMainThread]) {
      ++main_thread_posts_;
    }
    runner_->PostTask(std::move(task));
  }

  bool RunsTasksOnCurrentThread() override { return runner_->RunsTasksOnCurrentThread(); }

  int MainThreadPosts() const { return main_thread_posts_; }

 private:
  lynx::fml::RefPtr<lynx::fml::TaskRunner> runner_;
  int main_thread_posts_ = 0;
};

}  // namespace

@interface VSyncMonitorDarwinTest : XCTestCase
@end

@implementation VSyncMonitorDarwinTest

- (void)testFrameSourceSelectionAndReschedulingOnTASM {
  XCTAssertTrue([NSThread isMainThread]);
  lynx::fml::Thread tasm("VSyncMonitorDarwinTest");
  for (bool use_ui_vsync : {false, true}) {
    FrameTaskRunnerDelegate delegate(tasm.GetTaskRunner());
    auto runner = lynx::fml::MakeRefCounted<lynx::fml::TaskRunner>(nullptr);
    runner->SetDelegate(&delegate);
    auto monitor = VSyncMonitor::Create(use_ui_vsync);
    XCTestExpectation *next_frame = [self expectationWithDescription:@"next TASM frame"];

    tasm.GetTaskRunner()->PostSyncTask([&] {
      monitor->BindTaskRunner(runner);
      monitor->Init();
      monitor->ScheduleVSyncSecondaryCallback(1, [runner, weak_monitor = monitor->weak_from_this(),
                                                  next_frame](int64_t start, int64_t target) {
        XCTAssertTrue(runner->RunsTasksOnCurrentThread());
        XCTAssertFalse([NSThread isMainThread]);
        XCTAssertGreaterThan(start, 0);
        XCTAssertGreaterThan(target, start);
        if (auto monitor = weak_monitor.lock()) {
          monitor->ScheduleVSyncSecondaryCallback(
              1, [runner, start, next_frame](int64_t next_start, int64_t) {
                XCTAssertTrue(runner->RunsTasksOnCurrentThread());
                XCTAssertGreaterThan(next_start, start);
                [next_frame fulfill];
              });
        }
      });
    });

    [self waitForExpectations:@[ next_frame ] timeout:5];
    tasm.GetTaskRunner()->PostSyncTask([&] { monitor.reset(); });
    XCTAssertEqual(delegate.MainThreadPosts(), use_ui_vsync ? 2 : 0);
  }
}

- (void)testSharedHighRefreshFrameWithMergedRunnerAndDestroyedSubscriber {
  XCTAssertTrue([NSThread isMainThread]);
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();

  auto destroyed = VSyncMonitor::Create(true);
  destroyed->BindToCurrentThread();
  destroyed->SetHighRefreshRate();
  destroyed->AsyncRequestVSync(
      [](int64_t, int64_t) { XCTFail(@"A destroyed monitor must drop its pending frame"); });
  auto weak_destroyed = destroyed->weak_from_this();
  destroyed.reset();
  XCTAssertTrue(weak_destroyed.expired());

  XCTestExpectation *frame = [self expectationWithDescription:@"shared UI frame"];
  frame.expectedFulfillmentCount = 2;
  std::array<int64_t, 2> starts{};
  std::array<std::shared_ptr<VSyncMonitor>, 2> monitors = {VSyncMonitor::Create(true),
                                                           VSyncMonitor::Create(true)};
  for (std::size_t i = 0; i < monitors.size(); ++i) {
    auto &monitor = monitors[i];
    monitor->BindToCurrentThread();
    monitor->SetHighRefreshRate();
    monitor->AsyncRequestVSync([i, &starts, frame](int64_t start, int64_t target) {
      XCTAssertTrue([NSThread isMainThread]);
      XCTAssertGreaterThan(target, start);
      starts[i] = start;
      [frame fulfill];
    });
  }

  [self waitForExpectations:@[ frame ] timeout:5];
  XCTAssertGreaterThan(starts[0], 0);
  XCTAssertEqual(starts[0], starts[1]);
}

@end
