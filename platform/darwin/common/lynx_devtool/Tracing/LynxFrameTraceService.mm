// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFrameTraceService.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#import "tracing/frame_trace_service.h"
#endif

@implementation LynxFrameTraceService {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  std::shared_ptr<lynx::trace::FrameTraceService> _frame_trace_service;
#endif
}

+ (instancetype)shareInstance {
  static LynxFrameTraceService *instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (id)init {
  if ([super init]) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
    _frame_trace_service = std::make_shared<lynx::trace::FrameTraceService>();
#endif
  }
  return self;
}

- (void)initializeService {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  if (_frame_trace_service) {
    _frame_trace_service->Initialize();
  }
#endif
}

- (void)screenshot:(NSString *)snapshot {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  if (_frame_trace_service) {
    _frame_trace_service->SendScreenshots(snapshot ? std::string([snapshot UTF8String]) : "");
  }
#endif
}

- (void)FPSTrace:(const uint64_t)startTime withEndTime:(const uint64_t)endTime {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  if (_frame_trace_service) {
    _frame_trace_service->SendFPSData(startTime, endTime);
  }
#endif
}
@end
