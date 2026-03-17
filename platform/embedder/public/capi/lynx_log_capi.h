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

// Callback function type for receiving Lynx log messages.
// @param level The severity level of the log message.
// @param tag A string identifying the source or context of the log message.
// @param msg The actual log message content.
typedef void (*lynx_log_callback_t)(lynx_log_level_e level, const char* tag,
                                    const char* msg);

// Initializes the Lynx logging system with a custom callback.
// @param callback The function pointer to be invoked for logging.
LYNX_CAPI_EXPORT void lynx_log_init(lynx_log_callback_t callback);

// Sets the minimum severity level for logs to be output.
// Logs with a level lower than this will be ignored.
// @param min_log_level The minimum log level to set.
LYNX_CAPI_EXPORT void lynx_log_set_minimum_level(
    lynx_log_level_e min_log_level);

// Retrieves the current minimum log level.
// @return The current minimum log level.
LYNX_CAPI_EXPORT lynx_log_level_e lynx_log_get_minimum_level();

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
