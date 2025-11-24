// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOG_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOG_CAPI_H_

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

// Defines the log levels for the Lynx logging system.
typedef enum lynx_log_level_e {
  LYNX_LOG_VERBOSE = 0,
  LYNX_LOG_DEBUG,
  LYNX_LOG_INFO,
  LYNX_LOG_WARNING,
  LYNX_LOG_ERROR,
  LYNX_LOG_FATAL,
} lynx_log_level_e;

// Writes a log message to the Lynx logging system.
// This function supports printf-style formatting.
// @param level The log level for the message.
// @param tag A tag to identify the source of the log message.
// @param format The printf-style format string.
// @param ... Additional arguments for the format string.
LYNX_CAPI_EXPORT void lynx_log_write_detailed(lynx_log_level_e level,
                                              const char* tag, const char* file,
                                              int line, const char* format,
                                              ...);

#ifdef __FILE_NAME__
#define __LYNX_CAPI_LOG_FILE__ __FILE_NAME__
#else
#define __LYNX_CAPI_LOG_FILE__ __FILE__
#endif

// The primary logging macro for external C-API consumers.
// This captures the file and line number automatically.
// Example: LYNX_CAPI_LOG(LYNX_LOG_INFO, "MyTag", "User ID: %d", 123);
#define LYNX_CAPI_LOG(level, tag, format, ...)                          \
  lynx_log_write_detailed(level, tag, __LYNX_CAPI_LOG_FILE__, __LINE__, \
                          format, ##__VA_ARGS__)

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOG_CAPI_H_
