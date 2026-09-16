// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_ASYNC_PREPARATION_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_ASYNC_PREPARATION_H_

#include <process.h>
#include <windows.h>

#include <memory>
#include <mutex>
#include <utility>

namespace clay {
namespace egl {

// One-shot background resource preparation. The factory must not access this
// object. Finish concurrent consumers before destroying it.
template <typename T>
class AsyncPreparation {
 public:
  using Factory = std::unique_ptr<T> (*)();

  ~AsyncPreparation() { Discard(); }

  bool Start(Factory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
      return false;
    }
    if (job_) {
      return true;
    }
    auto job = std::make_unique<Job>(factory);
    job->thread = reinterpret_cast<HANDLE>(
        ::_beginthreadex(nullptr, 0, Run, job.get(), 0, nullptr));
    if (!job->thread) {
      return false;
    }
    job_ = std::move(job);
    return true;
  }

  std::unique_ptr<T> WaitAndTake() {
    std::unique_ptr<Job> job;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
      job = std::move(job_);
    }
    if (!job) {
      return nullptr;
    }
    job->Join();
    return std::move(job->result);
  }

  void Discard() { WaitAndTake(); }

 private:
  struct Job {
    explicit Job(Factory factory) : factory(factory) {}
    ~Job() { Join(); }
    void Join() {
      if (thread) {
        ::WaitForSingleObject(thread, INFINITE);
        ::CloseHandle(thread);
        thread = nullptr;
      }
    }
    Factory factory;
    HANDLE thread = nullptr;
    std::unique_ptr<T> result;
  };

  static unsigned __stdcall Run(void* context) {
    auto* job = static_cast<Job*>(context);
    job->result = job->factory();
    return 0;
  }

  std::mutex mutex_;
  bool closed_ = false;
  std::unique_ptr<Job> job_;
};

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_ASYNC_PREPARATION_H_
