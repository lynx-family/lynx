// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_ANDROID_TIMING_COLLECTOR_ANDROID_H_
#define CORE_SERVICES_TIMING_HANDLER_ANDROID_TIMING_COLLECTOR_ANDROID_H_

#include <memory>
#include <string>

#include "core/services/timing_handler/timing_collector_platform_impl.h"

namespace lynx {
namespace tasm {
namespace timing {

class TimingCollectorAndroid : public TimingCollectorPlatformImpl {
 public:
  static bool RegisterJNI(JNIEnv* env);
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_ANDROID_TIMING_COLLECTOR_ANDROID_H_
