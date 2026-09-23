// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/v8/v8_isolate_wrapper_impl.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "libplatform/libplatform.h"
#if defined(OS_WIN)
#include "base/include/string/string_conversion_win.h"
#include "core/base/utils/paths_win.h"
#elif defined(OS_OSX)
#include "core/base/utils/paths_mac.h"
#endif

namespace lynx {
namespace runtime {
namespace js {
namespace {

class FMLForegroundTaskRunner : public v8::TaskRunner {
 public:
  explicit FMLForegroundTaskRunner(fml::RefPtr<fml::TaskRunner> task_runner)
      : task_runner_(std::move(task_runner)),
        active_(std::make_shared<std::atomic_bool>(true)) {}

  void PostTask(std::unique_ptr<v8::Task> task) override {
    PostDelayedTask(std::move(task), 0);
  }

  void PostNonNestableTask(std::unique_ptr<v8::Task> task) override {
    PostTask(std::move(task));
  }

  void PostDelayedTask(std::unique_ptr<v8::Task> task,
                       double delay_in_seconds) override {
    auto active = active_;
    task_runner_->PostDelayedTask(
        [active, task = std::move(task)]() mutable {
          if (active->load(std::memory_order_acquire)) {
            task->Run();
          }
        },
        fml::TimeDelta::FromSecondsF(delay_in_seconds));
  }

#if V8_MAJOR_VERSION >= 11
  void PostNonNestableDelayedTask(std::unique_ptr<v8::Task> task,
                                  double delay_in_seconds) override {
    PostDelayedTask(std::move(task), delay_in_seconds);
  }
#endif

  void PostIdleTask(std::unique_ptr<v8::IdleTask> task) override {}

  bool IdleTasksEnabled() override { return false; }

  bool NonNestableTasksEnabled() const override { return true; }

#if V8_MAJOR_VERSION >= 11
  bool NonNestableDelayedTasksEnabled() const override { return true; }
#endif

  void Deactivate() { active_->store(false, std::memory_order_release); }

 private:
  fml::RefPtr<fml::TaskRunner> task_runner_;
  std::shared_ptr<std::atomic_bool> active_;
};

class V8PlatformAdapter : public v8::Platform {
 public:
  V8PlatformAdapter() : platform_(v8::platform::NewDefaultPlatform()) {}

  void RegisterIsolate(v8::Isolate* isolate,
                       fml::RefPtr<fml::TaskRunner> task_runner) {
    std::lock_guard<std::mutex> lock(mutex_);
    foreground_task_runners_.emplace(
        isolate,
        std::make_shared<FMLForegroundTaskRunner>(std::move(task_runner)));
  }

  void DeactivateIsolate(v8::Isolate* isolate) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = foreground_task_runners_.find(isolate);
    if (it != foreground_task_runners_.end()) {
      it->second->Deactivate();
    }
  }

  void UnregisterIsolate(v8::Isolate* isolate) {
    std::lock_guard<std::mutex> lock(mutex_);
    foreground_task_runners_.erase(isolate);
  }

  v8::PageAllocator* GetPageAllocator() override {
    return platform_->GetPageAllocator();
  }

  int NumberOfWorkerThreads() override {
    return platform_->NumberOfWorkerThreads();
  }

  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner(
      v8::Isolate* isolate) override {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = foreground_task_runners_.find(isolate);
    if (it != foreground_task_runners_.end()) {
      return it->second;
    }
    auto* message_loop = fml::MessageLoop::IsInitializedForCurrentThread();
    if (message_loop != nullptr) {
      auto task_runner = std::make_shared<FMLForegroundTaskRunner>(
          message_loop->GetTaskRunner());
      foreground_task_runners_.emplace(isolate, task_runner);
      return task_runner;
    }
    return platform_->GetForegroundTaskRunner(isolate);
  }

  void CallOnWorkerThread(std::unique_ptr<v8::Task> task) override {
    platform_->CallOnWorkerThread(std::move(task));
  }

  void CallBlockingTaskOnWorkerThread(std::unique_ptr<v8::Task> task) override {
    platform_->CallBlockingTaskOnWorkerThread(std::move(task));
  }

#if V8_MAJOR_VERSION >= 11
  void CallLowPriorityTaskOnWorkerThread(
      std::unique_ptr<v8::Task> task) override {
    platform_->CallLowPriorityTaskOnWorkerThread(std::move(task));
  }
#endif

  void CallDelayedOnWorkerThread(std::unique_ptr<v8::Task> task,
                                 double delay_in_seconds) override {
    platform_->CallDelayedOnWorkerThread(std::move(task), delay_in_seconds);
  }

#if V8_MAJOR_VERSION < 11
  void CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) override {
    GetForegroundTaskRunner(isolate)->PostTask(std::unique_ptr<v8::Task>(task));
  }

  void CallDelayedOnForegroundThread(v8::Isolate* isolate, v8::Task* task,
                                     double delay_in_seconds) override {
    GetForegroundTaskRunner(isolate)->PostDelayedTask(
        std::unique_ptr<v8::Task>(task), delay_in_seconds);
  }
#else
  std::unique_ptr<v8::JobHandle> CreateJob(
      v8::TaskPriority priority,
      std::unique_ptr<v8::JobTask> job_task) override {
    return platform_->CreateJob(priority, std::move(job_task));
  }
#endif

  double MonotonicallyIncreasingTime() override {
    return platform_->MonotonicallyIncreasingTime();
  }

  double CurrentClockTimeMillis() override {
    return platform_->CurrentClockTimeMillis();
  }

  v8::TracingController* GetTracingController() override {
    return platform_->GetTracingController();
  }

 private:
  std::unique_ptr<v8::Platform> platform_;
  std::mutex mutex_;
  std::unordered_map<v8::Isolate*, std::shared_ptr<FMLForegroundTaskRunner>>
      foreground_task_runners_;
};

V8PlatformAdapter& GetV8Platform() {
  static V8PlatformAdapter platform;
  return platform;
}

}  // namespace

V8IsolateInstanceImpl::V8IsolateInstanceImpl() = default;

V8IsolateInstanceImpl::~V8IsolateInstanceImpl() {
  if (isolate_ != nullptr) {
    GetV8Platform().DeactivateIsolate(isolate_);
    isolate_->Dispose();
    GetV8Platform().UnregisterIsolate(isolate_);
    LOGI("lynx ~V8IsolateInstance");
  }
}

std::once_flag flag;
void V8IsolateInstanceImpl::InitIsolate(const char* arg, bool useSnapshot) {
  LOGI("lynx V8IsolateInstanceImpl::InitIsolate");
  auto& platform = GetV8Platform();
  std::call_once(flag, [&platform]() {
    v8::V8::InitializeICU();
#if defined(OS_WIN)
    auto [_, path] = lynx::base::GetModuleDirectoryPath();
    std::string path_ansi = lynx::base::Utf8ToANSIOrOEM(path);
    v8::V8::InitializeExternalStartupData((path_ansi + "\\").c_str());
#elif defined(OS_OSX)
    auto [_, path] = lynx::common::GetResourceDirectoryPath();
    v8::V8::InitializeExternalStartupData((path + "\\").c_str());
#endif

    v8::V8::InitializePlatform(&platform);
    v8::V8::Initialize();
  });
  v8::Isolate::CreateParams create_params;
  create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  isolate_ = v8::Isolate::New(create_params);
  auto* message_loop = fml::MessageLoop::IsInitializedForCurrentThread();
  if (message_loop != nullptr) {
    platform.RegisterIsolate(isolate_, message_loop->GetTaskRunner());
  } else {
    LOGW(
        "V8 isolate was created without an fml message loop; foreground "
        "tasks will use V8's default task runner.");
  }
}

v8::Isolate* V8IsolateInstanceImpl::Isolate() const { return isolate_; }

}  // namespace js

}  // namespace runtime
}  // namespace lynx
