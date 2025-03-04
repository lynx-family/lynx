// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREAD_ONCE_TASK_H_
#define CORE_BASE_THREAD_ONCE_TASK_H_

#include <functional>
#include <future>
#include <mutex>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace base {

// For now, the OnceTask class is a key class to impl the parallel flush of
// elements. At present, when exec the parallel flush of elements, each element
// will post a task into the thread pool. After completion, some reduce tasks
// will be returned to be executed in the tasm thread. In addition, after all
// tasks have been thrown into the thread pool, the tasm thread will try to
// transfer the accumulated tasks to the tasm thread for execution. Since this
// logic is not a general thread pool logic, we need to encapsulate a OnceTask
// class to implement this capability. In the future, other parallel scenarios
// can also use this class to perform similar operations. Based on this class
// and thread pool, a simple code example is shown in the once_task_unittest.cc
template <typename T, typename... Args>
class OnceTask : public fml::RefCountedThreadSafeStorage {
 public:
  OnceTask(base::MoveOnlyClosure<void, Args...> task, std::future<T> future)
      : started_(false), task_(std::move(task)), future_(std::move(future)) {}
  ~OnceTask() override = default;

  std::future<T>& GetFuture() { return future_; }

  void ReleaseSelf() const override { delete this; };

  // Returning true indicates that run task in the current thread, while
  // returning false indicates that the task has already been started in another
  // thread.
  bool Run(Args... arguments) {
    bool expected_run = false;
    if (started_.compare_exchange_strong(expected_run, true)) {
      task_(std::forward<Args>(arguments)...);
      return true;
    }
    return false;
  }

 private:
  std::atomic_bool started_;
  base::MoveOnlyClosure<void, Args...> task_;
  std::future<T> future_;
};

template <typename T, typename... Args>
using OnceTaskRefptr = fml::RefPtr<OnceTask<T, Args...>>;

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREAD_ONCE_TASK_H_
