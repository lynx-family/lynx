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

#if defined(OS_IOS) || defined(OS_ANDROID)
#include "base/include/fml/platform/thread_config_setter.h"
#endif

#if defined(OS_ANDROID)
#include "base/include/platform/android/jni_utils.h"
#endif

namespace lynx {
namespace fml {

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
  base::closure setup_thread = [&latch, &runner, &loop_impl, setter,
                                &config]() {
    auto additional_setup_closure = config.additional_setup_closure;
    if (additional_setup_closure) {
      (*additional_setup_closure)();
    }
    setter(config);
    auto& loop = fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop_impl = loop.GetLoopImpl();
    runner = loop.GetTaskRunner();
    latch.Signal();
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
  thread_ = std::make_unique<std::thread>(std::move(setup_thread));
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
  thread_->join();
}

}  // namespace fml
}  // namespace lynx
