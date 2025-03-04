// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxCallStackUtil.h"

#include "base/include/debug/backtrace.h"

@implementation LynxCallStackUtil

+ (NSString*)getCallStack {
  std::string call_stack;
  call_stack = lynx::base::debug::GetBacktraceInfo(call_stack);
  return [NSString stringWithCString:call_stack.c_str() encoding:NSUTF8StringEncoding];
}

+ (NSString*)getCallStack:(NSException*)e {
  if (!e) {
    return nil;
  }
  // stack message
  NSArray<NSString*>* stacks = e.callStackSymbols;
  NSMutableString* stackInfo = [NSMutableString string];
  // save the first 20 lines of information on the stack
  for (NSUInteger i = 0; i < MIN(stacks.count, (NSUInteger)20); ++i) {
    [stackInfo appendFormat:@"%@\n", stacks[i]];
  }
  return stackInfo;
}

@end
