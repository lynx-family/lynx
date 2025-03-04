// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxBacktrace.h"
#import <Foundation/Foundation.h>
#include "base/include/debug/backtrace.h"

static LynxBacktraceFunction sLynxBacktraceFunction;

namespace lynx {
namespace base {
namespace debug {
class FunctionBacktraceDelegate : public BacktraceDelegate {
 public:
  ~FunctionBacktraceDelegate() override {}
  std::string TraceLog(std::string& msg, int skipDepth) override {
    LynxBacktraceFunction backtraceFunction = LynxGetBacktraceFunction();
    if (backtraceFunction != nil) {
      NSString* backtrace =
          backtraceFunction([NSString stringWithCString:msg.c_str() encoding:NSUTF8StringEncoding],
                            [[NSNumber numberWithInt:(skipDepth + 1)] unsignedIntegerValue]);
      if (backtrace != nil) {
        return [backtrace UTF8String];
      }
    }
    return std::string();
  }
};
}  // namespace debug
}  // namespace base
}  // namespace lynx

LynxBacktraceFunction LynxGetBacktraceFunction(void) { return sLynxBacktraceFunction; }

void LynxSetBacktraceFunction(LynxBacktraceFunction backtraceFunction) {
  sLynxBacktraceFunction = backtraceFunction;
// we can get backtrace with symbol in debug without Heimdallr
#ifndef DEBUG
  lynx::base::debug::SetBacktraceDelegate(new lynx::base::debug::FunctionBacktraceDelegate);
#endif
}
