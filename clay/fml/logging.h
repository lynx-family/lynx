// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_LOGGING_H_
#define CLAY_FML_LOGGING_H_

// This file is a compatibility bridge to the new logging implementation in
// lynx/base/include/log/logging.h. All FML_... macros are redefined in terms
// of the new BASE_... and standard macros.

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/log/logging.h"
#include "build/build_config.h"
// #include "clay/fml/log_level.h"
namespace fml {
using LogSeverity = ::lynx::base::logging::LogSeverity;
constexpr LogSeverity LOG_INFO = ::lynx::base::logging::LOG_INFO;
constexpr LogSeverity LOG_WARNING = ::lynx::base::logging::LOG_WARNING;
constexpr LogSeverity LOG_ERROR = ::lynx::base::logging::LOG_ERROR;
constexpr LogSeverity LOG_FATAL = ::lynx::base::logging::LOG_FATAL;
}  // namespace fml

// --- Stream-based Macros ---

#define FML_LOG(severity) BASE_LOG(severity)

#ifndef NDEBUG
#define FML_DLOG(severity) FML_LOG(severity)
#else
#define FML_DLOG(severity)                            \
  true ? (void)0                                      \
       : ::lynx::base::logging::LogMessageVoidify() & \
             ::lynx::base::logging::NullLogStream()
#endif

// VLOG is for verbose debugging. We map it to a disabled stream to ensure
// compatibility without adding complexity, as it's not critical.
#define FML_VLOG(verbose_level)                       \
  true ? (void)0                                      \
       : ::lynx::base::logging::LogMessageVoidify() & \
             ::lynx::base::logging::NullLogStream()

// --- Assertion and Check Macros ---

#define FML_CHECK(condition) CHECK(condition)
#define FML_DCHECK(condition) DCHECK(condition)

// --- Utility Macros ---

#define FML_UNREACHABLE() NOTREACHED()
#define FML_UNIMPLEMENTED() UNIMPLEMENTED()

// --- Deprecated Macros ---
// These macros are part of the old implementation and are no longer needed.
// Redefined here for any code that might still use them transitively.
#define FML_EAT_STREAM_PARAMETERS(ignored)            \
  true ? (void)0                                      \
       : ::lynx::base::logging::LogMessageVoidify() & \
             ::lynx::base::logging::NullLogStream()

#define FML_LOG_STREAM(severity) LOG_STREAM(severity)
#define FML_VLOG_STREAM(verbose_level)                 \
  (true ? (void)0                                      \
        : ::lynx::base::logging::LogMessageVoidify() & \
              ::lynx::base::logging::NullLogStream())
#define FML_LOG_IS_ON(severity) LOG_IS_ON(severity)
#define FML_VLOG_IS_ON(verbose_level) false

#endif  // CLAY_FML_LOGGING_H_
