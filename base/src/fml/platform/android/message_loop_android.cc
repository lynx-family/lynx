// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/android/message_loop_android.h"

#include <fcntl.h>
#include <unistd.h>

#include "base/include/fml/platform/linux/timerfd.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/include/platform/android/scoped_java_ref.h"

namespace lynx {
namespace fml {

fml::RefPtr<MessageLoopImpl> MessageLoopImpl::Create(void* platform_loop) {
  return fml::MakeRefCounted<MessageLoopAndroid>();
}

static constexpr int kClockType = CLOCK_MONOTONIC;

static lynx::base::android::ScopedGlobalJavaRef<jclass>* g_looper_class =
    nullptr;
static jmethodID g_looper_prepare_method_ = nullptr;
static jmethodID g_looper_loop_method_ = nullptr;
static jmethodID g_looper_my_looper_method_ = nullptr;
static jmethodID g_looper_quit_method_ = nullptr;

static void LooperPrepare() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  env->CallStaticVoidMethod(g_looper_class->Get(), g_looper_prepare_method_);
}

static void LooperLoop() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  env->CallStaticVoidMethod(g_looper_class->Get(), g_looper_loop_method_);
}

static void LooperQuit() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto my_looper = env->CallStaticObjectMethod(g_looper_class->Get(),
                                               g_looper_my_looper_method_);
  if (my_looper != nullptr) {
    env->CallVoidMethod(my_looper, g_looper_quit_method_);
  }
}

static ALooper* AcquireLooperForThread() {
  ALooper* looper = ALooper_forThread();
  if (looper == nullptr) {
    // No looper has been configured for the current thread. Create one and
    // return the same.
    looper = ALooper_prepare(0);
  }

  // The thread already has a looper. Acquire a reference to the same and return
  // it.
  ALooper_acquire(looper);
  return looper;
}

MessageLoopAndroid::MessageLoopAndroid()
    : looper_(AcquireLooperForThread()),
      timer_fd_(::timerfd_create(kClockType, TFD_NONBLOCK | TFD_CLOEXEC)),
      running_(false) {
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(looper_.is_valid());
  LYNX_BASE_CHECK(timer_fd_.is_valid());
  static const int kWakeEvents = ALOOPER_EVENT_INPUT;

  ALooper_callbackFunc read_event_fd = [](int, int events, void* data) -> int {
    if (events & kWakeEvents) {
      reinterpret_cast<MessageLoopAndroid*>(data)->OnEventFired();
    }
    return 1;  // continue receiving callbacks
  };

  [[maybe_unused]] int add_result =
      ::ALooper_addFd(looper_.get(),          // looper
                      timer_fd_.get(),        // fd
                      ALOOPER_POLL_CALLBACK,  // ident
                      kWakeEvents,            // events
                      read_event_fd,          // callback
                      this                    // baton
      );
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(add_result == 1);
}

MessageLoopAndroid::~MessageLoopAndroid() {
  [[maybe_unused]] int remove_result =
      ::ALooper_removeFd(looper_.get(), timer_fd_.get());
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(remove_result == 1);
}

void MessageLoopAndroid::Run() {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(looper_.get() == ALooper_forThread());

  running_ = true;

  LooperPrepare();
  LooperLoop();
}

void MessageLoopAndroid::Terminate() {
  running_ = false;
  LooperQuit();
}

void MessageLoopAndroid::WakeUp(fml::TimePoint time_point) {
  [[maybe_unused]] bool result = TimerRearm(timer_fd_.get(), time_point);
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(result);
}

void MessageLoopAndroid::OnEventFired() {
  if (TimerDrain(timer_fd_.get())) {
    RunExpiredTasksNow();
  }
}

bool MessageLoopAndroid::Register(JNIEnv* env) {
  jclass clazz = env->FindClass("android/os/Looper");

  if (clazz == nullptr) {
    return false;
  }

  g_looper_class = new base::android::ScopedGlobalJavaRef<jclass>(env, clazz);

  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(!(g_looper_class->IsNull()));

  g_looper_prepare_method_ =
      env->GetStaticMethodID(g_looper_class->Get(), "prepare", "()V");
  LYNX_BASE_CHECK(g_looper_prepare_method_ != nullptr);

  g_looper_loop_method_ =
      env->GetStaticMethodID(g_looper_class->Get(), "loop", "()V");
  LYNX_BASE_CHECK(g_looper_loop_method_ != nullptr);

  g_looper_my_looper_method_ = env->GetStaticMethodID(
      g_looper_class->Get(), "myLooper", "()Landroid/os/Looper;");
  LYNX_BASE_CHECK(g_looper_my_looper_method_ != nullptr);

  g_looper_quit_method_ =
      env->GetMethodID(g_looper_class->Get(), "quit", "()V");

  return true;
}

}  // namespace fml
}  // namespace lynx
