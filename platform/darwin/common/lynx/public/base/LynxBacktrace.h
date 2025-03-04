// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXBACKTRACE_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXBACKTRACE_H_

#import <Foundation/Foundation.h>
#import "LynxDefines.h"

typedef NSString* (^LynxBacktraceFunction)(NSString* message, NSUInteger skippedDepth);

LYNX_EXTERN void LynxSetBacktraceFunction(LynxBacktraceFunction backtraceFunction);
LYNX_EXTERN LynxBacktraceFunction LynxGetBacktraceFunction(void);

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXBACKTRACE_H_
