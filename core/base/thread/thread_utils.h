// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_THREAD_THREAD_UTILS_H_
#define CORE_BASE_THREAD_THREAD_UTILS_H_

#include <string>

namespace lynx {
namespace base {

/**
 * @brief Get the current thread's name.
 *
 * @return std::string The name of the current thread as a C++ string. If the
 * thread name cannot be retrieved, the function returns "unknown".
 */
std::string GetCurrentThreadName();

}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_THREAD_THREAD_UTILS_H_
