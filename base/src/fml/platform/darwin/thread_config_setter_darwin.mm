// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#include <pthread.h>
#include "base/include/fml/platform/thread_config_setter.h"

namespace lynx {
namespace fml {

namespace {

void SetThreadPriority(int sched_priority, qos_class_t qos_class, double thread_priority) {
  pthread_set_qos_class_self_np(qos_class, 0);
  [[NSThread currentThread] setThreadPriority:thread_priority];
  sched_param param;
  int policy;
  pthread_t thread = pthread_self();
  if (pthread_getschedparam(thread, &policy, &param) == 0) {
    param.sched_priority = sched_priority;
    pthread_setschedparam(thread, policy, &param);
  }
}

}  // namespace

/// Inheriting ThreadConfigurer and use iOS platform thread API to configure the thread priorities
/// Using iOS platform thread API to configure thread priority
// thread_name |  sched_priority | threadPriority
// ui | 31 |  0.5
// js |  31->50 |  0.5->0.806452
// Layout | 31->50 |  0.5->0.806452
// TASM | 31->50 |  0.5->0.806452
void PlatformThreadPriority::Setter(const lynx::fml::Thread::ThreadConfig& config) {
  @autoreleasepool {
    // set thread name
    lynx::fml::Thread::SetCurrentThreadName(config);

    // set thread priority
    // The sched_priority values are based on the Darwin kernel's scheduling priorities.
    // The range for default policy (SCHED_OTHER) is typically 0-63.
    // A higher number means a higher priority.
    // 4: A low priority, suitable for background tasks that are not time-sensitive.
    // 31: The default priority for user-interactive threads.
    // 46: A high priority, just below the typical main thread priority (47),
    //     suitable for important, user-initiated work.
    switch (config.priority) {
      case lynx::fml::Thread::ThreadPriority::BACKGROUND:
      case lynx::fml::Thread::ThreadPriority::LOW:
        SetThreadPriority(4, QOS_CLASS_BACKGROUND, 0.0);
        break;
      case lynx::fml::Thread::ThreadPriority::NORMAL:
        SetThreadPriority(31, QOS_CLASS_DEFAULT, 0.5);
        break;
      case lynx::fml::Thread::ThreadPriority::HIGH:
        SetThreadPriority(46, QOS_CLASS_USER_INITIATED, 1.0);
        break;
    }
  }
}

}  // namespace fml
}  // namespace lynx
