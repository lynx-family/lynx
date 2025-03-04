// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/js_thread_config_getter.h"

#include <string>

#include "base/include/fml/thread.h"

namespace lynx {
namespace base {

fml::Thread::ThreadConfig GetJSThreadConfig(const std::string& worker_name) {
  return fml::Thread::ThreadConfig{worker_name,
                                   fml::Thread::ThreadPriority::HIGH, nullptr};
}
}  // namespace base
}  // namespace lynx
