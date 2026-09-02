// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <deque>
#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/message_loop_impl.h"
#include "clay/testing/testing.h"
#include "clay/ui/resource/image_cache.h"

namespace clay {
namespace {

#if defined(ENABLE_PLATFORM_DECODE) || defined(OS_WIN) || defined(OS_MAC)

class ManualTaskRunner final : public fml::TaskRunner {
 public:
  static fml::RefPtr<ManualTaskRunner> Create(
      bool runs_tasks_on_current_thread) {
    return fml::AdoptRef(new ManualTaskRunner(runs_tasks_on_current_thread));
  }

  void PostTask(lynx::base::closure task) override {
    tasks_.push_back(std::move(task));
  }

  bool RunsTasksOnCurrentThread() override {
    return runs_tasks_on_current_thread_;
  }

  size_t GetPendingTaskCount() const { return tasks_.size(); }

  void RunUntilIdle() {
    while (!tasks_.empty()) {
      auto task = std::move(tasks_.front());
      tasks_.pop_front();
      task();
    }
  }

 private:
  explicit ManualTaskRunner(bool runs_tasks_on_current_thread)
      : TaskRunner(fml::RefPtr<fml::MessageLoopImpl>()),
        runs_tasks_on_current_thread_(runs_tasks_on_current_thread) {}

  bool runs_tasks_on_current_thread_;
  std::deque<lynx::base::closure> tasks_;
};

class TrackedImage {
 public:
  TrackedImage(size_t bytes, std::shared_ptr<size_t> destruction_count)
      : bytes_(bytes), destruction_count_(std::move(destruction_count)) {}

  ~TrackedImage() { ++(*destruction_count_); }

  size_t GetGraphicsImageAllocSize() const { return bytes_; }

 private:
  size_t bytes_;
  std::shared_ptr<size_t> destruction_count_;
};

TEST(ImageCacheTest, ClearUsesInjectedCleanupRunnerAndDefersDestruction) {
  auto ui_task_runner = ManualTaskRunner::Create(true);
  auto cleanup_task_runner = ManualTaskRunner::Create(false);
  auto destruction_count = std::make_shared<size_t>(0);
  auto cache = std::make_shared<ImageCache<TrackedImage>>(ui_task_runner,
                                                          cleanup_task_runner);

  cache->StoreImage(
      1, "image", std::make_shared<TrackedImage>(1, destruction_count), false);
  cache->ClearCache();

  EXPECT_EQ(ui_task_runner->GetPendingTaskCount(), 0u);
  EXPECT_EQ(cleanup_task_runner->GetPendingTaskCount(), 1u);
  EXPECT_EQ(*destruction_count, 0u);

  cleanup_task_runner->RunUntilIdle();
  EXPECT_EQ(*destruction_count, 1u);

  cache->ClearCache();
  EXPECT_EQ(cleanup_task_runner->GetPendingTaskCount(), 0u);
}

TEST(ImageCacheTest, CapacityEvictionUsesInjectedCleanupRunner) {
  auto ui_task_runner = ManualTaskRunner::Create(true);
  auto cleanup_task_runner = ManualTaskRunner::Create(false);
  auto destruction_count = std::make_shared<size_t>(0);
  auto cache = std::make_shared<ImageCache<TrackedImage>>(ui_task_runner,
                                                          cleanup_task_runner);
  const size_t image_count = kImageCacheMaxBytes / kMaxItemBytes + 1;

  for (size_t index = 0; index < image_count; ++index) {
    cache->StoreImage(
        index, std::to_string(index),
        std::make_shared<TrackedImage>(kMaxItemBytes, destruction_count),
        false);
  }

  EXPECT_GT(cleanup_task_runner->GetPendingTaskCount(), 0u);
  EXPECT_EQ(*destruction_count, 0u);

  cleanup_task_runner->RunUntilIdle();
  EXPECT_GT(*destruction_count, 0u);
  EXPECT_LT(*destruction_count, image_count);

  cache->ClearCache();
  cleanup_task_runner->RunUntilIdle();
  EXPECT_EQ(*destruction_count, image_count);
}

#endif

}  // namespace
}  // namespace clay
