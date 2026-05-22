// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_LOGGING_H_
#define CLAY_FML_LOGGING_H_

#include <sstream>

#include "base/include/fml/macros.h"
#include "build/build_config.h"
#include "clay/fml/log_level.h"

namespace fml {

typedef void (*LogMessageCallback)(const char* log_level, const char* file,
                                   const int line, const char* log_info);

void SetLogMessageCallback(LogMessageCallback callback);

class LogMessageVoidify {
 public:
  void operator&(std::ostream&) {}
};

class LogMessage {
 public:
  LogMessage(LogSeverity severity, const char* file, int line,
             const char* condition);
  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  std::ostringstream stream_;
  const LogSeverity severity_;
  const char* file_;
  const int line_;

  BASE_DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

// Gets the FML_VLOG default verbosity level.
int GetVlogVerbosity();

// Returns true if |severity| is at or above the current minimum log level.
// LOG_FATAL and above is always true.
bool ShouldCreateLogMessage(LogSeverity severity);

[[noreturn]] void KillProcess();

}  // namespace fml

#define FML_LOG_STREAM(severity) \
  ::fml::LogMessage(::fml::LOG_##severity, __FILE__, __LINE__, nullptr).stream()

#define FML_LAZY_STREAM(stream, condition) \
  !(condition) ? (void)0 : ::fml::LogMessageVoidify() & (stream)

#define FML_EAT_STREAM_PARAMETERS(ignored) \
  true || (ignored)                        \
      ? (void)0                            \
      : ::fml::LogMessageVoidify() &       \
            ::fml::LogMessage(::fml::LOG_FATAL, 0, 0, nullptr).stream()

#define FML_LOG_IS_ON(severity) \
  (::fml::ShouldCreateLogMessage(::fml::LOG_##severity))

#define FML_LOG(severity) \
  FML_LAZY_STREAM(FML_LOG_STREAM(severity), FML_LOG_IS_ON(severity))

#define FML_CHECK(condition)                                              \
  FML_LAZY_STREAM(                                                        \
      ::fml::LogMessage(::fml::LOG_FATAL, __FILE__, __LINE__, #condition) \
          .stream(),                                                      \
      !(condition))

#define FML_VLOG_IS_ON(verbose_level) \
  ((verbose_level) <= ::fml::GetVlogVerbosity())

// The VLOG macros log with negative verbosities.
#define FML_VLOG_STREAM(verbose_level) \
  ::fml::LogMessage(-verbose_level, __FILE__, __LINE__, nullptr).stream()

#define FML_VLOG(verbose_level) \
  FML_LAZY_STREAM(FML_VLOG_STREAM(verbose_level), FML_VLOG_IS_ON(verbose_level))

#ifndef NDEBUG
#define FML_DLOG(severity) FML_LOG(severity)
#define FML_DCHECK(condition) FML_CHECK(condition)
#else
#define FML_DLOG(severity) FML_EAT_STREAM_PARAMETERS(true)
#define FML_DCHECK(condition) FML_EAT_STREAM_PARAMETERS(condition)
#endif

#define FML_UNREACHABLE()                          \
  {                                                \
    FML_LOG(ERROR) << "Reached unreachable code."; \
    ::fml::KillProcess();                          \
  }

#define FML_UNIMPLEMENTED()                                               \
  FML_DLOG(ERROR) << "Unimplemented detail in " << __FUNCTION__ << " of " \
                  << __FILE__;

#endif  // CLAY_FML_LOGGING_H_
