// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_THREAD_CONFIG_SETTER_H_
#define BASE_INCLUDE_FML_PLATFORM_THREAD_CONFIG_SETTER_H_

#include "base/include/fml/thread.h"

namespace lynx {
namespace fml {

class PlatformThreadPriority {
 public:
  static void Setter(const lynx::fml::Thread::ThreadConfig& config);
};
}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_PLATFORM_THREAD_CONFIG_SETTER_H_
