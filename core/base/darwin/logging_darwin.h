// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DARWIN_LOGGING_DARWIN_H_
#define CORE_BASE_DARWIN_LOGGING_DARWIN_H_

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace logging {

constexpr const char* kLynxLogLevels[] = {"V", "D", "I", "W", "E", "F"};

void InitLynxLoggingNative(bool enable_devtools);

void SetLynxLogMinLevel(int min_level);

void InternalLogNative(const char* file, int32_t line, int level,
                       const char* message);

void PrintLogMessageByLogDelegate(LogMessage* msg, const char* tag);

}  // namespace logging
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_DARWIN_LOGGING_DARWIN_H_
