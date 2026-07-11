// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/concurrent_message_loop_backend.h"
#include "base/src/fml/concurrent_loop_backend_std.h"
#if defined(OS_HARMONY)
#include "base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h"
#endif
#include "gtest/gtest.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace lynx {
namespace fml {
namespace {

// Fake backend: each PostTask spawns a fresh thread.
class FakeBackend : public ConcurrentLoopBackend {
 public:
  explicit FakeBackend(size_t worker_count) : worker_count_(worker_count) {}

  void PostTask(base::closure task) override {
    threads_.emplace_back([this, t = std::move(task)]() {
      g_current = this;
      t();
      g_current = nullptr;
    });
  }
  bool RunsTasksOnCurrentThreadWorker() const override {
    return g_current == this;
  }
  size_t GetWorkerCount() const override { return worker_count_; }
  void Terminate() override {
    for (auto& t : threads_) {
      if (t.joinable()) t.join();
    }
    threads_.clear();
  }

  static thread_local ConcurrentLoopBackend* g_current;

 private:
  size_t worker_count_;
  std::vector<std::thread> threads_;
};
thread_local ConcurrentLoopBackend* FakeBackend::g_current = nullptr;

TEST(ConcurrentLoopBackendTest, PostTaskExecutesTaskExactlyOnce) {
  auto backend = std::make_unique<FakeBackend>(2);
  std::atomic<int> count{0};
  backend->PostTask([&] { count.fetch_add(1); });
  backend->Terminate();
  EXPECT_EQ(count.load(), 1);
}

TEST(ConcurrentLoopBackendTest, RunsTasksOnCurrentThreadWorkerInsideTask) {
  auto backend = std::make_unique<FakeBackend>(1);
  std::atomic<bool> inside{false};
  std::atomic<bool> flag_inside{false};
  backend->PostTask([&] {
    inside.store(backend->RunsTasksOnCurrentThreadWorker());
    flag_inside.store(FakeBackend::g_current == backend.get());
  });
  backend->Terminate();
  EXPECT_TRUE(inside.load());
  EXPECT_TRUE(flag_inside.load());
  EXPECT_FALSE(backend->RunsTasksOnCurrentThreadWorker());
}

TEST(ConcurrentMessageLoopShutdownFallback,
     PostTaskRunsSynchronouslyOnCallerAfterShutdown) {
  auto loop = std::make_shared<ConcurrentMessageLoop>("shutdown-fallback",
                                                   Thread::ThreadPriority::NORMAL,
                                                   1);
  loop->Terminate();

  std::atomic<bool> ran{false};
  std::atomic<std::thread::id> ran_on_id{};
  const std::thread::id caller_id = std::this_thread::get_id();

  loop->PostTask([&] {
    ran.store(true);
    ran_on_id.store(std::this_thread::get_id());
  });

  EXPECT_TRUE(ran.load());
  EXPECT_EQ(ran_on_id.load(), caller_id);
  EXPECT_FALSE(loop->RunsTasksOnCurrentThreadWorker());
}

// Regression: PostTask after Terminate must run synchronously on the
// caller's thread (not crash, not enqueue into a destroyed backend).
// The facade-level test above covers the facade's shutdown_ fallback;
// this exercises the backend's own contract.
TEST(ConcurrentLoopBackendStdTest, PostTaskAfterTerminateRunsSync) {
  auto backend = std::make_unique<ConcurrentLoopBackendStd>(
      "post-term-std", Thread::ThreadPriority::NORMAL, 1);
  backend->Terminate();

  std::atomic<bool> ran{false};
  backend->PostTask([&] { ran.store(true); });
  EXPECT_TRUE(ran.load());
}

#if defined(OS_HARMONY)
TEST(ConcurrentLoopBackendFFRTTest, PostTaskAfterTerminateRunsSync) {
  auto backend = std::make_unique<ConcurrentLoopBackendFFRT>(
      "post-term-ffrt", Thread::ThreadPriority::NORMAL, 1);
  backend->Terminate();

  std::atomic<bool> ran{false};
  backend->PostTask([&] { ran.store(true); });
  EXPECT_TRUE(ran.load());
}
#endif

}  // namespace
}  // namespace fml
}  // namespace lynx
