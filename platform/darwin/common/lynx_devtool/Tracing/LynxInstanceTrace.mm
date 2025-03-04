// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxInstanceTrace.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#import "tracing/platform/instance_trace_plugin_darwin.h"
#endif

@implementation LynxInstanceTrace {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  std::unique_ptr<lynx::trace::InstanceTracePluginDarwin> _instance_trace_plugin;
#endif
}

+ (instancetype)shareInstance {
  static LynxInstanceTrace *instance;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (id)init {
  if (self = [super init]) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
    _instance_trace_plugin = std::make_unique<lynx::trace::InstanceTracePluginDarwin>();
#endif
  }
  return self;
}

- (intptr_t)getInstanceTracePlugin {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  return reinterpret_cast<intptr_t>(_instance_trace_plugin.get());
#endif
  return 0;
}

@end
