// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_DEBUG_BACKTRACE_H_
#define BASE_INCLUDE_DEBUG_BACKTRACE_H_

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#if OS_IOS
#include <execinfo.h>
#endif

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace debug {

class BacktraceDelegate {
 public:
  virtual ~BacktraceDelegate() {}
  virtual std::string TraceLog(std::string& msg, int skipDepth) = 0;
};

void SetBacktraceDelegate(BacktraceDelegate* delegate);

// IOS
std::string GetBacktraceInfo(std::string& error_message);

}  // namespace debug
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_DEBUG_BACKTRACE_H_
