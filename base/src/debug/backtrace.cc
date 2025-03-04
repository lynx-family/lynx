// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/debug/backtrace.h"

#include <utility>

namespace lynx {
namespace base {
namespace debug {

BacktraceDelegate* g_backtrace_delegate = nullptr;
void SetBacktraceDelegate(BacktraceDelegate* delegate) {
  if (g_backtrace_delegate) {
    delete g_backtrace_delegate;
  }
  g_backtrace_delegate = delegate;
}
// IOS
std::string GetBacktraceInfo(std::string& error_message) {
  if (g_backtrace_delegate) {
    std::string traceInfo = g_backtrace_delegate->TraceLog(error_message, 2);
    if (!traceInfo.empty()) {
      return traceInfo;
    }
  }
  // in debug
#if OS_IOS
  error_message.append("\n\n");
  constexpr int max = 30;
  void* buffer[max];
  int stack_num = backtrace(buffer, max);
  char** stacktrace = backtrace_symbols(buffer, stack_num);
  if (stacktrace == nullptr) {
    return "";
  }
  // begin from 2 can throw backtrace for AddBackTrace and LynxException
  for (int i = 2; i < stack_num; ++i) {
    // make order begin with 0
    int order = i - 2;
    int offset = order >= 10 ? 3 : 2;
    error_message.append(std::to_string(order))
        .append(stacktrace[i] + offset)
        .append("\n");
  }
  free(stacktrace);
#endif
  return error_message;
}

}  // namespace debug
}  // namespace base
}  // namespace lynx
