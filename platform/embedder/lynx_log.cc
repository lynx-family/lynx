// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <stdarg.h>
#include <stdio.h>

#include <string>
#include <vector>

#include "base/include/log/logging.h"
#include "platform/embedder/public/capi/lynx_log_capi.h"

namespace {
static lynx_log_callback_t g_lynx_log_callback = nullptr;
static lynx_log_level_e g_min_log_level = LYNX_LOG_VERBOSE;

void lynx_log_write(unsigned int level, const char* tag, const char* msg) {
  if (!g_lynx_log_callback || level < g_min_log_level) {
    return;
  }
  g_lynx_log_callback(static_cast<lynx_log_level_e>(level), tag, msg);
}
}  // namespace

LYNX_EXTERN_C void lynx_log_init(lynx_log_callback_t callback) {
  g_lynx_log_callback = callback;
  auto get_log_callback = []() -> lynx::base::alog_write_func_ptr {
    return lynx_log_write;
  };
  lynx::base::logging::InitLynxLogging(get_log_callback, nullptr, true);
}

LYNX_EXTERN_C void lynx_log_set_minimum_level(lynx_log_level_e min_log_level) {
  g_min_log_level = min_log_level;
  lynx::base::logging::SetMinLogLevel(static_cast<int>(min_log_level));
}

LYNX_EXTERN_C lynx_log_level_e lynx_log_get_minimum_level() {
  return static_cast<lynx_log_level_e>(lynx::base::logging::GetMinLogLevel());
}

LYNX_EXTERN_C void lynx_log_write_detailed(lynx_log_level_e level,
                                           const char* tag, const char* file,
                                           int line, const char* format, ...) {
  if (level < lynx::base::logging::GetMinLogLevel()) {
    return;
  }

  va_list args;
  va_start(args, format);

  // Create a copy for the first vsnprintf which may consume the args.
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(nullptr, 0, format, args_copy);
  va_end(args_copy);

  if (size < 0) {
    va_end(args);
    return;
  }

  // Use a stack-allocated buffer for small messages to avoid heap allocation.
  char stack_buffer[256];
  if (static_cast<size_t>(size) < sizeof(stack_buffer)) {
    vsnprintf(stack_buffer, sizeof(stack_buffer), format, args);
    // Directly stream the C-style string buffer.
    switch (level) {
      case LYNX_LOG_VERBOSE:
        LOGV("[" << tag << ":" << file << ":" << line << "] " << stack_buffer);
        break;
      case LYNX_LOG_DEBUG:
        LOGD("[" << tag << ":" << file << ":" << line << "] " << stack_buffer);
        break;
      case LYNX_LOG_INFO:
        LOGI("[" << tag << ":" << file << ":" << line << "] " << stack_buffer);
        break;
      case LYNX_LOG_WARNING:
        LOGW("[" << tag << ":" << file << ":" << line << "] " << stack_buffer);
        break;
      case LYNX_LOG_ERROR:
        LOGE("[" << tag << ":" << file << ":" << line << "] " << stack_buffer);
        break;
      default:
        LOGI("[" << tag << ":" << file << ":" << line << "] " << stack_buffer);
        break;
    }
  } else {
    // For larger messages, fall back to heap allocation.
    std::vector<char> heap_buffer(size + 1);
    vsnprintf(heap_buffer.data(), heap_buffer.size(), format, args);
    // Directly stream the C-style string buffer.
    switch (level) {
      case LYNX_LOG_VERBOSE:
        LOGV("[" << tag << ":" << file << ":" << line << "] "
                 << heap_buffer.data());
        break;
      case LYNX_LOG_DEBUG:
        LOGD("[" << tag << ":" << file << ":" << line << "] "
                 << heap_buffer.data());
        break;
      case LYNX_LOG_INFO:
        LOGI("[" << tag << ":" << file << ":" << line << "] "
                 << heap_buffer.data());
        break;
      case LYNX_LOG_WARNING:
        LOGW("[" << tag << ":" << file << ":" << line << "] "
                 << heap_buffer.data());
        break;
      case LYNX_LOG_ERROR:
        LOGE("[" << tag << ":" << file << ":" << line << "] "
                 << heap_buffer.data());
        break;
      default:
        LOGI("[" << tag << ":" << file << ":" << line << "] "
                 << heap_buffer.data());
        break;
    }
  }

  va_end(args);
}
