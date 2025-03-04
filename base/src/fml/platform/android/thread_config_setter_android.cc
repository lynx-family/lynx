// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <sys/resource.h>

#include "base/include/fml/platform/thread_config_setter.h"

namespace lynx {
namespace fml {

// Inheriting ThreadConfigurer and use Android platform thread API to
// configure the thread priorities
void PlatformThreadPriority::Setter(
    const lynx::fml::Thread::ThreadConfig& config) {
  // set thread name
  lynx::fml::Thread::SetCurrentThreadName(config);

  // set thread priority
  switch (config.priority) {
    case lynx::fml::Thread::ThreadPriority::BACKGROUND: {
      // android.os.Process.THREAD_PRIORITY_BACKGROUND == 10
      if (::setpriority(PRIO_PROCESS, 0, 10) != 0) {
        // TODO(zhengsenyao): Uncomment the following log code
        // LOGE("Failed to lower the priority of the task runner");
      }
      break;
    }
    case lynx::fml::Thread::ThreadPriority::LOW: {
      // android.os.Process.THREAD_PRIORITY_BACKGROUND == 10
      if (::setpriority(PRIO_PROCESS, 0, 10) != 0) {
        // TODO(zhengsenyao): Uncomment the following log code
        // LOGE("Failed to lower the priority of the task runner");
      }
      break;
    }
    case lynx::fml::Thread::ThreadPriority::NORMAL: {
      // android.os.Process.ANDROID_PRIORITY_NORMAL == 0
      // android.os.Process.ANDROID_PRIORITY_MORE_FAVORABLE == -1
      if (::setpriority(PRIO_PROCESS, 0, -1) != 0) {
        // TODO(zhengsenyao): Uncomment the following log code
        // LOGE("Unable to assign normal priority to task runner");
      }
      break;
    }
    case lynx::fml::Thread::ThreadPriority::HIGH: {
      // android.os.Process.THREAD_PRIORITY_URGENT_DISPLAY == -8
      // Android describes -8 as "most important display threads, for
      // compositing the screen and retrieving input events".
      if (::setpriority(PRIO_PROCESS, 0, -8) != 0) {
        // Defensive fallback. Depending on the OEM, it may not be possible
        // to set priority to -8.
        if (::setpriority(PRIO_PROCESS, 0, -2) != 0) {
          // TODO(zhengsenyao): Uncomment the following log code
          // LOGE("Unable to assign higher priority to task runner");
        }
      }
      break;
    }
  }
}

}  // namespace fml
}  // namespace lynx
