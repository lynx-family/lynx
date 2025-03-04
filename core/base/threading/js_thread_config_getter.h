// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREADING_JS_THREAD_CONFIG_GETTER_H_
#define CORE_BASE_THREADING_JS_THREAD_CONFIG_GETTER_H_

#include <string>

#include "base/include/fml/thread.h"

namespace lynx {
namespace base {
fml::Thread::ThreadConfig GetJSThreadConfig(const std::string& worker_name);
}
}  // namespace lynx

#endif  // CORE_BASE_THREADING_JS_THREAD_CONFIG_GETTER_H_
