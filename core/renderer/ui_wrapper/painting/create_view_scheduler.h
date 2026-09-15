// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_CREATE_VIEW_SCHEDULER_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_CREATE_VIEW_SCHEDULER_H_

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <utility>

#include "base/include/concurrent_queue.h"
#include "core/base/thread/once_task.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace tasm {

// Threading contract: ai/docs/shared_create_view_scheduler.md.
template <typename Result>
class CreateViewScheduler {
 public:
  class Task final : public base::OnceTask<Result> {
   public:
    Task(base::closure prepare, std::future<Result> future)
        : base::OnceTask<Result>(std::move(prepare), std::move(future)) {}

   private:
    friend class CreateViewScheduler;
    using base::OnceTask<Result>::GetFuture;
    using base::OnceTask<Result>::Run;
    void PrepareOnUI() {
      if (preparing_on_ui_) {
        return;
      }
      preparing_on_ui_ = true;
      this->Run();
      preparing_on_ui_ = false;
    }
    // UI-thread only. Claim before Run: preparation may reenter Consume.
    bool consumed_ = false;
    bool preparing_on_ui_ = false;
  };
  using TaskRef = fml::RefPtr<Task>;

 private:
  using Queue = base::ConcurrentQueue<TaskRef>;

 public:
  class Batch {
   public:
    explicit Batch(typename Queue::IterableContainer tasks)
        : tasks_(std::move(tasks)), next_(tasks_.begin()) {}

   private:
    friend class CreateViewScheduler;
    TaskRef TakeNext() {
      if (next_ == tasks_.end()) {
        return nullptr;
      }
      // Advance and release this entry before any reentrant JNI callback.
      return std::move(*next_++);
    }
    typename Queue::IterableContainer tasks_;
    typename Queue::Iterator next_;
  };
  using BatchRef = std::shared_ptr<Batch>;
  using Dispatch = std::function<void(base::closure)>;

  CreateViewScheduler()
      : CreateViewScheduler([](base::closure task) {
          base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
              std::move(task), base::ConcurrentTaskType::HIGH_PRIORITY);
        }) {}

  explicit CreateViewScheduler(Dispatch dispatch)
      : dispatch_(std::move(dispatch)) {}

  TaskRef Schedule(base::MoveOnlyClosure<Result> prepare, bool defer = false) {
    std::promise<Result> promise;
    auto future = promise.get_future();
    auto task = fml::MakeRefCounted<Task>(
        [prepare = std::move(prepare), promise = std::move(promise)]() mutable {
          auto callback = std::move(prepare);
          promise.set_value(callback());
        },
        std::move(future));
    if (defer) {
      deferred_.Push(task);
    } else {
      DispatchTask(task);
    }
    return task;
  }

  void DispatchDeferred() {
    auto tasks = deferred_.PopAll();
    for (auto& task : tasks) {
      DispatchTask(std::move(task));
    }
  }

  BatchRef TakeBatch() {
    auto tasks = scheduled_.ReversePopAll();
    return tasks.empty() ? nullptr : std::make_shared<Batch>(std::move(tasks));
  }
  void ActivateBatch(BatchRef batch) { active_batch_ = std::move(batch); }
  void ResetBatch() { active_batch_.reset(); }

  // An engaged optional is the sole consumption, even if Result itself is null.
  // Return nullopt if already claimed or if preparation is on this UI stack.
  // The latter leaves the result available for consumption after preparation.
  std::optional<Result> Consume(TaskRef task) {
    if (task->consumed_ || task->preparing_on_ui_) {
      return std::nullopt;
    }
    task->consumed_ = true;
    // Preparation may destroy the scheduler; retain state before invoking it.
    auto batch = active_batch_;
    task->PrepareOnUI();
    auto& future = task->GetFuture();
    while (future.wait_for(std::chrono::seconds(0)) !=
               std::future_status::ready &&
           batch) {
      auto other = batch->TakeNext();
      if (!other) {
        break;
      }
      other->PrepareOnUI();  // Never get another task's future.
    }
    return future.get();
  }

 private:
  void DispatchTask(TaskRef task) {
    dispatch_([task]() { task->Run(); });
    scheduled_.Push(std::move(task));
  }

  Dispatch dispatch_;
  Queue deferred_;
  Queue scheduled_;
  BatchRef active_batch_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_CREATE_VIEW_SCHEDULER_H_
