// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LOG_LOGGING_BASE_H_
#define BASE_INCLUDE_LOG_LOGGING_BASE_H_

#include <cstdint>
#include <vector>

#include "base/include/base_export.h"

namespace lynx {

void InitLynxBaseLog(bool is_print_log_to_all_channel);

enum LynxLogSource {
  LynxLogSourceNaitve = 1 << 0,
  LynxLogSourceJS = 1 << 1,
};

typedef void (*LynxLogFunction)(int level, const char* message);

struct LynxLogDelegate {
  LynxLogFunction log_function;
  int min_log_level;
  bool should_format_message;
  LynxLogSource accept_source;
  int64_t accept_runtime_id;

  LynxLogDelegate() {
    log_function = nullptr;
    min_log_level = -1;
    should_format_message = true;
    accept_source = LynxLogSource(LynxLogSourceNaitve | LynxLogSourceJS);
    accept_runtime_id = -1;
  }
};

BASE_EXPORT void SetDebugLoggingDelegate(LynxLogDelegate* delegate);
BASE_EXPORT int AddLoggingDelegate(LynxLogDelegate* delegate);
BASE_EXPORT LynxLogDelegate* GetLoggingDelegate(int delegate_id);
BASE_EXPORT std::vector<LynxLogDelegate*> GetLoggingDelegates();
BASE_EXPORT void RemoveLoggingDelegate(int delegate_id);
BASE_EXPORT void SetMinimumLoggingLevel(int min_log_level);
BASE_EXPORT void SetJSLogsFromExternalChannels(bool is_open);
BASE_EXPORT int GetMinimumLoggingLevel();

BASE_EXPORT int LynxSetLogFunction(LynxLogFunction log_function);
BASE_EXPORT LynxLogFunction LynxGetLogFunction();

}  // namespace lynx
#endif  // BASE_INCLUDE_LOG_LOGGING_BASE_H_
