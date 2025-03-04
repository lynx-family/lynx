// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/task_runner_manufactor.h"

#include <algorithm>
#include <condition_variable>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO(zhengsenyao): move it to tasm source_set
#include "base/include/compiler_specific.h"
#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "core/base/threading/js_thread_config_getter.h"
#include "core/renderer/utils/lynx_env.h"  // nogncheck

// TODO(qiuxian):merge vsync loop to normal loop.
#ifdef OS_ANDROID
#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/android/device_utils_android.h"
#include "core/base/android/message_loop_android_vsync.h"
#include "core/base/threading/task_runner_vsync.h"
#elif defined(OS_IOS)
#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/darwin/message_loop_darwin_vsync.h"
#include "core/base/threading/task_runner_vsync.h"
#endif

#ifdef OS_WIN
#include "base/include/fml/platform/win/task_runner_win32.h"
#endif

namespace lynx {
namespace base {

namespace {

inline size_t GetMaxThreadsAllowed() {
  // Reserve two threads for the UI thread and the JS thread.
  constexpr size_t kReservedThreadCount = 2;
  constexpr size_t kMinThreadCount = 2;

  static size_t cpu_cores =
      static_cast<size_t>(std::thread::hardware_concurrency());

  static size_t remaining_thread_count =
      cpu_cores >= kReservedThreadCount ? cpu_cores - kReservedThreadCount : 0;

  static size_t max_thread_count =
      std::max(remaining_thread_count, kMinThreadCount);

  return max_thread_count;
}

inline bool& HasInit() {
  static bool has_init = false;
  return has_init;
}

inline std::condition_variable& GetUIInitCV() {
  static base::NoDestructor<std::condition_variable> ui_thread_init_cv_;
  return *ui_thread_init_cv_;
}

#if !defined(OS_WIN)
// fix for unused function error.
inline std::mutex& GetUIThreadMutex() {
  static base::NoDestructor<std::mutex> ui_thread_init_mutex;
  return *ui_thread_init_mutex;
}
#endif

inline fml::RefPtr<fml::TaskRunner>& GetUITaskRunner() {
// win need use fml::TaskRunnerWin32
#ifdef OS_WIN
  static base::NoDestructor<fml::RefPtr<fml::TaskRunner>> runner(
      fml::TaskRunnerWin32::Create());
#else
  // other platform, UIThread::Init init ui thread loop and set runner here
  static base::NoDestructor<fml::RefPtr<fml::TaskRunner>> runner;
#endif
  return *runner;
}

class ThreadGroup {
 public:
  fml::RefPtr<fml::TaskRunner> GetTaskRunner(size_t thread_index) {
    size_t index = thread_index % max_count_;
    {
      std::lock_guard<std::mutex> lock(thread_group_mutex_);
      auto it = thread_groups_.find(index);
      if (it == thread_groups_.end()) {
        auto new_thread = std::make_unique<fml::Thread>(
            GetJSThreadConfig(prefix_name_ + std::to_string(index)));
        auto task_runner =
            fml::MakeRefCounted<fml::TaskRunner>(new_thread->GetLoop());
        thread_groups_.emplace(index, std::move(new_thread));
        LOGI("ThreadGroup for " << prefix_name_ << ", max_count:" << max_count_
                                << ", new thread for index:" << index);
        return task_runner;
      }
      return fml::MakeRefCounted<fml::TaskRunner>(it->second->GetLoop());
    }
  }

 private:
  friend class base::NoDestructor<ThreadGroup>;
  size_t max_count_;
  std::string prefix_name_;
  std::unordered_map<size_t, std::unique_ptr<fml::Thread>> thread_groups_;
  std::mutex thread_group_mutex_;

  ThreadGroup(const std::string& prefix_name, size_t max_count) {
    max_count_ = max_count;
    prefix_name_ = prefix_name;
  }
};

inline size_t GetMultiTASMThreadCacheSize() {
  static size_t max_thread_count = GetMaxThreadsAllowed();
  static size_t multi_tasm_thread_cache_size =
      std::min(static_cast<size_t>(tasm::LynxEnv::GetInstance().GetLongEnv(
                   tasm::LynxEnv::Key::MULTI_TASM_THREAD_SIZE,
                   static_cast<int>(max_thread_count))),
               max_thread_count);
  return multi_tasm_thread_cache_size;
}

inline size_t GetMultiLayoutThreadCacheSize() {
  // The Layout process takes less time in most scenarios, so it is not
  // necessary to set a high number of threads
  static const size_t kDefaultLayoutThreadCacheMaxSize = 3;
  static size_t max_thread_count =
      std::min(kDefaultLayoutThreadCacheMaxSize, GetMaxThreadsAllowed());

  static size_t multi_layout_thread_cache_size =
      std::min(static_cast<size_t>(tasm::LynxEnv::GetInstance().GetLongEnv(
                   tasm::LynxEnv::Key::MULTI_LAYOUT_THREAD_SIZE,
                   static_cast<int>(max_thread_count))),
               max_thread_count);
  return multi_layout_thread_cache_size;
}

inline ThreadGroup& GetJSGroupThreadCache(const std::string& prefix_name,
                                          size_t max_count) {
  static base::NoDestructor<ThreadGroup> js_thread_cache{
      prefix_name,
      max_count > 0 ? max_count : std::thread::hardware_concurrency()};
  return *js_thread_cache;
}

inline ThreadGroup& GetTASMThreadCache(const std::string& prefix_name) {
  static size_t tasm_thread_max_count = GetMultiTASMThreadCacheSize();
  static base::NoDestructor<ThreadGroup> tasm_thread_cache{
      prefix_name, tasm_thread_max_count};
  return *tasm_thread_cache;
}

inline ThreadGroup& GetLayoutThreadCache(const std::string& prefix_name) {
  static size_t layout_thread_max_count = GetMultiLayoutThreadCacheSize();
  static base::NoDestructor<ThreadGroup> layout_thread_cache{
      prefix_name, layout_thread_max_count};
  return *layout_thread_cache;
}

static size_t GetConcurrentLoopHighPriorityWorkerCount() {
  constexpr size_t min_count = 1;
  const size_t max_count =
      std::max(min_count, size_t(std::thread::hardware_concurrency()));
  size_t count = max_count;
#ifdef OS_ANDROID
  size_t percent =
      std::clamp(tasm::LynxEnv::GetInstance().GetLongEnv(
                     tasm::LynxEnv::Key::
                         CONCURRENT_LOOP_HIGH_PRIORITY_WORKER_COUNT_PERCENT,
                     0),
                 0L, 100L);
  if (percent > 0) {
    count = max_count * percent / 100;
  } else if (!android::DeviceUtilsAndroid::Is64BitDevice()) {
    count = max_count / 2;
  }
#endif
  return std::clamp(count, min_count, max_count);
}

}  // namespace

inline fml::RefPtr<fml::TaskRunner>& GetUIVSyncTaskRunner() {
  static base::NoDestructor<fml::RefPtr<fml::TaskRunner>> runner;
  return *runner;
}

fml::RefPtr<fml::TaskRunner>& UIThread::GetRunner(
    bool enable_vsync_aligned_msg_loop) {
#if !defined(OS_WIN)
  if (!HasInit()) {
    LOGI("Waiting for UIThread to initialize.");
    std::unique_lock<std::mutex> local_lock(GetUIThreadMutex());
    GetUIInitCV().wait(local_lock);
  }
#endif
  return enable_vsync_aligned_msg_loop ? GetUIVSyncTaskRunner()
                                       : GetUITaskRunner();
}

void UIThread::Init(void* platform_loop) {
  if (HasInit()) {
    return;
  }
  GetUITaskRunner() =
      fml::MessageLoop::EnsureInitializedForCurrentThread(platform_loop)
          .GetTaskRunner();
  // TODO(qiuxian): TaskRunnerVSync will be removed after inject vsync to normal
  // loop.
#ifdef OS_ANDROID
  GetUIVSyncTaskRunner() = fml::MakeRefCounted<base::TaskRunnerVSync>(
      fml::MakeRefCounted<android::MessageLoopAndroidVSync>());
#elif defined(OS_IOS)
  GetUIVSyncTaskRunner() = fml::MakeRefCounted<base::TaskRunnerVSync>(
      fml::MakeRefCounted<darwin::MessageLoopDarwinVSync>());
#endif
  HasInit() = true;
  GetUIInitCV().notify_all();
}

TaskRunnerManufactor::TaskRunnerManufactor(ThreadStrategyForRendering strategy,
                                           bool enable_multi_tasm_thread,
                                           bool enable_multi_layout_thread,
                                           bool enable_vsync_aligned_msg_loop,
                                           bool enable_async_thread_cache,
                                           std::string js_group_thread_name)
    : thread_strategy_(strategy),
      enable_multi_tasm_thread_(enable_multi_tasm_thread),
      js_group_thread_name_(std::move(js_group_thread_name)) {
  LOGI("TaskRunnerManufactor setThreadStrategy:"
       << strategy << ", multi_tasm:" << enable_multi_tasm_thread
       << ", async_thread_cache:" << enable_async_thread_cache);
  static size_t current_label = 0;
  label_ = ++current_label;

  switch (strategy) {
    case ALL_ON_UI: {
      StartUIThread(enable_vsync_aligned_msg_loop);
      StartJSThread();
      CreateTASMRunner(ui_task_runner_->GetLoop(),
                       enable_vsync_aligned_msg_loop);
      layout_task_runner_ = tasm_task_runner_;
    } break;
    case MOST_ON_TASM: {
      StartUIThread(enable_vsync_aligned_msg_loop);
      StartJSThread();
      CreateTASMRunner(StartTASMThread(), enable_vsync_aligned_msg_loop);
      layout_task_runner_ = tasm_task_runner_;
    } break;
    case PART_ON_LAYOUT: {
      StartUIThread(enable_vsync_aligned_msg_loop);
      StartJSThread();
      CreateTASMRunner(ui_task_runner_->GetLoop(),
                       enable_vsync_aligned_msg_loop);
      StartLayoutThread(enable_multi_layout_thread);
    } break;
    case MULTI_THREADS: {
      StartUIThread(enable_vsync_aligned_msg_loop);
      StartJSThread();
      CreateTASMRunner(StartTASMThread(), enable_vsync_aligned_msg_loop);
      StartLayoutThread(enable_multi_layout_thread);
    } break;
    default:
      break;
  }
}

fml::RefPtr<fml::TaskRunner> TaskRunnerManufactor::GetJSRunner(
    const std::string& js_group_thread_name) {
  static constexpr const char* js_thread_name = "Lynx_JS";
  if (js_group_thread_name.empty()) {
    static base::NoDestructor<fml::Thread> js_thread(
        GetJSThreadConfig(js_thread_name));
    return js_thread->GetTaskRunner();
  } else {
    unsigned char group_thread_name_last_char =
        static_cast<unsigned char>(js_group_thread_name.back());
    return GetJSGroupThreadCache(js_thread_name,
                                 std::thread::hardware_concurrency())
        .GetTaskRunner(group_thread_name_last_char);
  }
}

fml::RefPtr<fml::TaskRunner> TaskRunnerManufactor::GetTASMTaskRunner() {
  return tasm_task_runner_;
}

fml::RefPtr<fml::TaskRunner> TaskRunnerManufactor::GetLayoutTaskRunner() {
  return layout_task_runner_;
}

fml::RefPtr<fml::TaskRunner> TaskRunnerManufactor::GetUITaskRunner() {
  return ui_task_runner_;
}

fml::RefPtr<fml::TaskRunner> TaskRunnerManufactor::GetJSTaskRunner() {
  return js_task_runner_;
}

ThreadStrategyForRendering TaskRunnerManufactor::GetManufactorStrategy() {
  return thread_strategy_;
}

void TaskRunnerManufactor::OnThreadStrategyUpdated(
    ThreadStrategyForRendering new_strategy) {
  thread_strategy_ = new_strategy;
}

void TaskRunnerManufactor::StartUIThread(bool enable_vsync_aligned_msg_loop) {
  ui_task_runner_ = UIThread::GetRunner(enable_vsync_aligned_msg_loop);
}

void TaskRunnerManufactor::CreateTASMRunner(
    fml::RefPtr<fml::MessageLoopImpl> loop,
    bool enable_vsync_aligned_msg_loop) {
#ifdef OS_WIN
  tasm_task_runner_ = ui_task_runner_;
  return;
#endif

#if defined(OS_ANDROID) || (OS_IOS)
  // Only Android supports vsync aligned message loop.
  tasm_task_runner_ =
      enable_vsync_aligned_msg_loop
          ? fml::MakeRefCounted<base::TaskRunnerVSync>(std::move(loop))
          : fml::MakeRefCounted<fml::TaskRunner>(std::move(loop));
  return;
#else
  tasm_task_runner_ = fml::MakeRefCounted<fml::TaskRunner>(std::move(loop));
#endif
}

fml::RefPtr<fml::MessageLoopImpl> TaskRunnerManufactor::StartTASMThread() {
  static constexpr const char* tasm_thread_name = "Lynx_TASM";
  if (enable_multi_tasm_thread_) {
    tasm_loop_ =
        GetTASMThreadCache(tasm_thread_name).GetTaskRunner(label_)->GetLoop();
  } else {
    static base::NoDestructor<fml::Thread> tasm_thread(
        fml::Thread::ThreadConfig(tasm_thread_name,
                                  fml::Thread::ThreadPriority::HIGH));
    tasm_loop_ = tasm_thread->GetLoop();
  }

  return tasm_loop_;
}

fml::RefPtr<fml::MessageLoopImpl> TaskRunnerManufactor::GetTASMLoop() {
  return tasm_loop_ ? tasm_loop_ : StartTASMThread();
}

void TaskRunnerManufactor::StartLayoutThread(bool enable_multi_layout_thread) {
  static constexpr const char* layout_thread_name = "Lynx_Layout";
  if (enable_multi_layout_thread) {
    layout_task_runner_ =
        GetLayoutThreadCache(layout_thread_name).GetTaskRunner(label_);
  } else {
    static base::NoDestructor<fml::Thread> layout_thread(
        fml::Thread::ThreadConfig(layout_thread_name,
                                  fml::Thread::ThreadPriority::HIGH));
    layout_task_runner_ = layout_thread->GetTaskRunner();
  }
}

void TaskRunnerManufactor::StartJSThread() {
#if LYNX_ENABLE_FROZEN_MODE
  js_task_runner_ = ui_task_runner_;
#else
  js_task_runner_ = GetJSRunner(js_group_thread_name_);
#endif
}

fml::Thread TaskRunnerManufactor::CreateJSWorkerThread(
    const std::string& worker_name) {
  std::string thread_name = std::string("Lynx_JS_Worker-") + worker_name;
  return fml::Thread(thread_name);
}

void TaskRunnerManufactor::PostTaskToConcurrentLoop(base::closure task,
                                                    ConcurrentTaskType type) {
  switch (type) {
    case ConcurrentTaskType::HIGH_PRIORITY: {
      static base::NoDestructor<fml::ConcurrentMessageLoop> high_priority_loop(
          "LynxHighTask", fml::Thread::ThreadPriority::HIGH,
          GetConcurrentLoopHighPriorityWorkerCount());
      high_priority_loop->PostTask(std::move(task));
    } break;
    case ConcurrentTaskType::NORMAL_PRIORITY: {
      GetNormalPriorityLoop().PostTask(std::move(task));
    } break;
  }
}

fml::ConcurrentMessageLoop& TaskRunnerManufactor::GetNormalPriorityLoop() {
  // TODO(zhoupeng.z): merge with thread pool in LynxThreadPool.java
  constexpr size_t normal_worker_count = 1;
  static base::NoDestructor<fml::ConcurrentMessageLoop> normal_priority_loop(
      "LynxNormalTask", fml::Thread::ThreadPriority::NORMAL,
      normal_worker_count);
  return *normal_priority_loop;
}

}  // namespace base
}  // namespace lynx
