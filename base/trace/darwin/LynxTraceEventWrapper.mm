// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTraceEventWrapper.h"
#include "base/trace/native/internal_trace_category.h"

NSString *LYNX_TRACE_CATEGORY_WRAPPER;

@interface LynxTraceEventWrapper : NSObject

@end

@implementation LynxTraceEventWrapper

+ (void)load {
  [LynxTraceEventWrapper initEventName];
}

+ (void)initEventName {
  LYNX_TRACE_CATEGORY_WRAPPER = [NSString stringWithUTF8String:INTERNAL_TRACE_CATEGORY];
}

@end
