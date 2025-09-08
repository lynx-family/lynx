// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LOG_LOGGING_DARWIN_H_
#define BASE_INCLUDE_LOG_LOGGING_DARWIN_H_

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace logging {

constexpr const char* kLynxLogLevels[] = {"V", "D", "I", "W", "E", "F"};

void InitLynxLoggingNative(void* log_address,
                           PlatformLogCallBack platform_log_channel,
                           bool print_logs_to_all_channels);

void SetLynxLogMinLevel(int min_level);

void InternalLogNative(const char* file, int32_t line, int level,
                       const char* message);

}  // namespace logging
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_LOG_LOGGING_DARWIN_H_
