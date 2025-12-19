// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "base/include/fml/thread.h"

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/src/fml/thread_name_setter.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <process.h>
#include <windows.h>
#endif

#if defined(OS_IOS) || defined(OS_ANDROID)
#include <sched.h>

#include "base/include/fml/platform/thread_config_setter.h"
#endif

#if defined(OS_ANDROID)
#include "base/include/platform/android/jni_utils.h"
#endif

namespace lynx {
namespace fml {

class ThreadHandle {
 public:
  ThreadHandle(base::closure&& function);
  ~ThreadHandle();

  void Join();

 private:
#if defined(OS_WIN)
  HANDLE thread_;
#else
  pthread_t thread_;
#endif
};

#if defined(OS_WIN)
ThreadHandle::ThreadHandle(base::closure&& function) {
  thread_ = (HANDLE*)_beginthreadex(
      nullptr, Thread::GetDefaultStackSize(),
      [](void* arg) -> unsigned {
        std::unique_ptr<base::closure> function(
            reinterpret_cast<base::closure*>(arg));
        (*function)();
        return 0;
      },
      new base::closure(std::move(function)), 0, nullptr);
  LYNX_BASE_CHECK(thread_ != nullptr);
}

void ThreadHandle::Join() { WaitForSingleObjectEx(thread_, INFINITE, FALSE); }

ThreadHandle::~ThreadHandle() { CloseHandle(thread_); }
#else

ThreadHandle::ThreadHandle(base::closure&& function) {
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  // Set stack size.
  int result = pthread_attr_setstacksize(&attr, Thread::GetDefaultStackSize());
  LYNX_BASE_CHECK(result == 0);

#if defined(OS_IOS)
  // Set scheduling policy and priority for the new thread.
  // This can fail if the user does not have permissions to do so. We will
  // not check the result of these calls and let the thread be created with
  // default attributes.
  if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0) {
    int policy;
    struct sched_param current_param;
    if (pthread_getschedparam(pthread_self(), &policy, &current_param) == 0) {
      if (pthread_attr_setschedpolicy(&attr, policy) == 0) {
        struct sched_param high_prio_param;
        high_prio_param.sched_priority = sched_get_priority_max(policy);
        pthread_attr_setschedparam(&attr, &high_prio_param);
      }
    }
  }
#endif

  result = pthread_create(
      &thread_, &attr,
      [](void* arg) -> void* {
        std::unique_ptr<base::closure> function(
            reinterpret_cast<base::closure*>(arg));
        (*function)();
        return nullptr;
      },
      new base::closure(std::move(function)));
  LYNX_BASE_CHECK(result == 0);
  result = pthread_attr_destroy(&attr);
  LYNX_BASE_CHECK(result == 0);
}

void ThreadHandle::Join() { pthread_join(thread_, nullptr); }

ThreadHandle::~ThreadHandle() = default;

#endif

void Thread::SetCurrentThreadName(const Thread::ThreadConfig& config) {
  SetThreadName(config.name);
}

Thread::Thread(const std::string& name) : Thread(ThreadConfig(name)) {}

#if defined(OS_IOS) || defined(OS_ANDROID)
Thread::Thread(const ThreadConfig& config)
    : Thread(PlatformThreadPriority::Setter, config) {}
#else
Thread::Thread(const ThreadConfig& config)
    : Thread(Thread::SetCurrentThreadName, config) {}
#endif

Thread::Thread(const ThreadConfigSetter& setter, const ThreadConfig& config)
    : joined_(false) {
  fml::AutoResetWaitableEvent latch;
  fml::RefPtr<fml::TaskRunner> runner;
  fml::RefPtr<fml::MessageLoopImpl> loop_impl;
  base::closure setup_thread = [&latch, &runner, &loop_impl, setter, config]() {
    auto additional_setup_closure = config.additional_setup_closure;
    if (additional_setup_closure) {
      (*additional_setup_closure)();
    }

    auto& loop = fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop_impl = loop.GetLoopImpl();
    runner = loop.GetTaskRunner();
    latch.Signal();
    setter(config);
    loop.Run();
    // hack, because we cannot detach vm within MessageLoop Terminate,
    // Terminate is called in Android Looper, the java code.
    // If we invoke attempting DetachCurrentThread within Terminate,
    // we will get another exception "attempting to detach while still running
    // code". so we must detach here, after the loop stop running.
#if defined(OS_ANDROID)
    lynx::base::android::DetachFromVM();
#endif
  };
  thread_ = std::make_unique<ThreadHandle>(std::move(setup_thread));
  latch.Wait();
  task_runner_ = runner;
  loop_ = loop_impl;
}

Thread::~Thread() { Join(); }

const fml::RefPtr<fml::TaskRunner>& Thread::GetTaskRunner() const {
  return task_runner_;
}

const fml::RefPtr<fml::MessageLoopImpl>& Thread::GetLoop() const {
  return loop_;
}

void Thread::Join() {
  if (joined_) {
    return;
  }
  joined_ = true;
  task_runner_->PostTask([]() { MessageLoop::GetCurrent().Terminate(); });
  thread_->Join();
}

size_t Thread::GetDefaultStackSize() { return 1024 * 1024 * 1; }

}  // namespace fml
}  // namespace lynx
