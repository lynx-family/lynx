// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_EXPIRED_ON_TASK_RUNNER_H_
#define CLAY_SHELL_COMMON_EXPIRED_ON_TASK_RUNNER_H_

#include <memory>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/task_runner.h"
#include "clay/fml/logging.h"

namespace clay {

template <typename T>
class ExpiredOnTaskRunner {
 public:
  ExpiredOnTaskRunner() : ExpiredOnTaskRunner(nullptr) {}

  explicit ExpiredOnTaskRunner(std::unique_ptr<T> ptr)
      : resource_ptr_(std::move(ptr)),
        task_runner_(nullptr),
        release_func_(nullptr) {}

  ExpiredOnTaskRunner(std::unique_ptr<T> ptr,
                      fml::RefPtr<fml::TaskRunner> task_runner)
      : resource_ptr_(std::move(ptr)),
        task_runner_(task_runner),
        release_func_(nullptr) {}

  ~ExpiredOnTaskRunner() { ReleaseSourcesOnTaskRunner(); }

  void SetTaskRunner(fml::RefPtr<fml::TaskRunner> task_runner) {
    FML_DCHECK(task_runner);
    task_runner_ = task_runner;
  }

  void SetReleaseFunc(const std::function<void(std::unique_ptr<T>)>& func) {
    FML_DCHECK(func);
    release_func_ = func;
  }

  ExpiredOnTaskRunner<T>& operator=(ExpiredOnTaskRunner<T>&& other) {
    if (&other == this) {
      return *this;
    }
    ReleaseSourcesOnTaskRunner();
    resource_ptr_ = std::move(other.resource_ptr_);
    task_runner_ = other.task_runner_;
    release_func_ = other.release_func_;
    other.Reset();
    return *this;
  }

  bool IsValid() { return resource_ptr_ && task_runner_ && release_func_; }

  T* operator->() { return resource_ptr_.get(); }

 private:
  void Reset() {
    resource_ptr_ = nullptr;
    task_runner_ = nullptr;
    release_func_ = nullptr;
  }

  void ReleaseSourcesOnTaskRunner() {
    if (!resource_ptr_ || !release_func_ || !task_runner_) {
      return;
    }
    fml::TaskRunner::RunNowOrPostTask(
        task_runner_, fml::MakeCopyable([ptr = std::move(resource_ptr_),
                                         func = release_func_]() mutable {
          func(std::move(ptr));
        }));
  }

  std::unique_ptr<T> resource_ptr_;
  fml::RefPtr<fml::TaskRunner> task_runner_;
  std::function<void(std::unique_ptr<T>)> release_func_;
};
}  // namespace clay
#endif  // CLAY_SHELL_COMMON_EXPIRED_ON_TASK_RUNNER_H_
