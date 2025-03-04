// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_TIMER_TIME_UTILS_H_
#define BASE_INCLUDE_TIMER_TIME_UTILS_H_

#include <time.h>

#include <cstdint>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
// This method should not be used except in unsatisfied scenarios.
uint64_t CurrentSystemTimeMilliseconds();
// This method should not be used except in unsatisfied scenarios.
uint64_t CurrentSystemTimeMicroseconds();
uint64_t CurrentTimeMicroseconds();
uint64_t CurrentTimeMilliseconds();
uint64_t CurrentThreadCPUTimeMicroseconds();
#if !defined(OS_WIN)
timespec ToTimeSpecFromNow(uint64_t interval_time);
#endif
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_TIMER_TIME_UTILS_H_
