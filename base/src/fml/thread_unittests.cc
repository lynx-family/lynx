// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/thread.h"
#include "build/build_config.h"

#if defined(OS_MACOSX) || defined(OS_LINUX) || defined(OS_ANDROID)
#define FLUTTER_PTHREAD_SUPPORTED 1
#else
#define FLUTTER_PTHREAD_SUPPORTED 0
#endif

#if FLUTTER_PTHREAD_SUPPORTED
#include <pthread.h>
#else
#endif

#include <memory>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {

TEST(Thread, CanStartAndEnd) {
  fml::Thread thread;
  ASSERT_TRUE(thread.GetTaskRunner());
}

TEST(Thread, CanStartAndEndWithExplicitJoin) {
  fml::Thread thread;
  ASSERT_TRUE(thread.GetTaskRunner());
  thread.Join();
}

TEST(Thread, HasARunningMessageLoop) {
  fml::Thread thread;
  bool done = false;
  thread.GetTaskRunner()->PostTask([&done]() { done = true; });
  thread.Join();
  ASSERT_TRUE(done);
}

#if FLUTTER_PTHREAD_SUPPORTED
TEST(Thread, ThreadNameCreatedWithConfig) {
  const std::string name = "Thread1";
  fml::Thread thread(name);

  bool done = false;
  thread.GetTaskRunner()->PostTask([&done, &name]() {
    done = true;
    // The thread name's length is set to 16 on Android & Linux
    char thread_name[16];
    pthread_t current_thread = pthread_self();
    pthread_getname_np(current_thread, thread_name, sizeof(thread_name));
    ASSERT_EQ(thread_name, name);
  });
  thread.Join();
  ASSERT_TRUE(done);
}

static void MockThreadConfigSetter(const fml::Thread::ThreadConfig& config) {
  // set thread name
  fml::Thread::SetCurrentThreadName(config);

  pthread_t tid = pthread_self();
  struct sched_param param;
  int policy = SCHED_OTHER;
  switch (config.priority) {
    case fml::Thread::ThreadPriority::HIGH:
      param.sched_priority = 10;
      break;
    default:
      param.sched_priority = 1;
  }
    // In linx system, sched_priority can only be set to 0 with SCHED_OTHER
    // policy.
#if defined(OS_LINUX)
  param.sched_priority = 0;
#endif
  pthread_setschedparam(tid, policy, &param);
}

TEST(Thread, ThreadPriorityCreatedWithConfig) {
  const std::string thread1_name = "Thread1";
  const std::string thread2_name = "Thread2";

  fml::Thread thread(MockThreadConfigSetter,
                     fml::Thread::ThreadConfig(
                         thread1_name, fml::Thread::ThreadPriority::NORMAL));
  bool done = false;

  struct sched_param param;
  int policy;
  thread.GetTaskRunner()->PostTask([&]() {
    done = true;
    char thread_name[16];
    pthread_t current_thread = pthread_self();
    pthread_getname_np(current_thread, thread_name, sizeof(thread_name));
    pthread_getschedparam(current_thread, &policy, &param);
    ASSERT_EQ(thread_name, thread1_name);
    ASSERT_EQ(policy, SCHED_OTHER);
    // In linx system, sched_priority can only be set to 0 with SCHED_OTHER
    // policy.
#if !defined(OS_LINUX)
    ASSERT_EQ(param.sched_priority, 1);
#else
    ASSERT_EQ(param.sched_priority, 0);
#endif
  });

  struct sched_param param2;
  fml::Thread thread2(MockThreadConfigSetter,
                      fml::Thread::ThreadConfig(
                          thread2_name, fml::Thread::ThreadPriority::HIGH));
  thread2.GetTaskRunner()->PostTask([&]() {
    done = true;
    char thread_name[16];
    pthread_t current_thread = pthread_self();
    pthread_getname_np(current_thread, thread_name, sizeof(thread_name));
    pthread_getschedparam(current_thread, &policy, &param2);
    ASSERT_EQ(thread_name, thread2_name);
    ASSERT_EQ(policy, SCHED_OTHER);
    // In linx system, sched_priority can only be set to 0 with SCHED_OTHER
    // policy.
#if !defined(OS_LINUX)
    ASSERT_EQ(param2.sched_priority, 10);
#else
    ASSERT_EQ(param2.sched_priority, 0);
#endif
  });
  thread.Join();
  ASSERT_TRUE(done);
}
#endif

}  // namespace lynx
