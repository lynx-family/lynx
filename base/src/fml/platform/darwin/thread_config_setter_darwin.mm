// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#include "base/include/fml/platform/thread_config_setter.h"

namespace lynx {
namespace fml {

/// Inheriting ThreadConfigurer and use iOS platform thread API to configure the thread priorities
/// Using iOS platform thread API to configure thread priority
// thread_name |  sched_priority | threadPriority
// ui | 31 |  0.5
// js |  31->50 |  0.5->0.806452
// Layout | 31->50 |  0.5->0.806452
// TASM | 31->50 |  0.5->0.806452
void PlatformThreadPriority::Setter(const lynx::fml::Thread::ThreadConfig& config) {
  // set thread name
  lynx::fml::Thread::SetCurrentThreadName(config);

  // set thread priority
  switch (config.priority) {
    case lynx::fml::Thread::ThreadPriority::BACKGROUND: {
      pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0);
      [[NSThread currentThread] setThreadPriority:0];
      break;
    }
    case lynx::fml::Thread::ThreadPriority::LOW: {
      pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0);
      [[NSThread currentThread] setThreadPriority:0];
      break;
    }
    case lynx::fml::Thread::ThreadPriority::NORMAL: {
      pthread_set_qos_class_self_np(QOS_CLASS_DEFAULT, 0);
      [[NSThread currentThread] setThreadPriority:0.5];
      break;
    }
    case lynx::fml::Thread::ThreadPriority::HIGH: {
      pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
      [[NSThread currentThread] setThreadPriority:1.0];
      sched_param param;
      int policy;
      pthread_t thread = pthread_self();
      if (!pthread_getschedparam(thread, &policy, &param)) {
        // It is common to see the main thread preempt current thread at priority 47.
        // so we set the child thread priority to 46(47-1);
        param.sched_priority = 46;
        pthread_setschedparam(thread, policy, &param);
      }
      break;
    }
  }
}

}  // namespace fml
}  // namespace lynx
