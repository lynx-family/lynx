// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxBase/LynxLog.h>
#include <map>

#import <LynxServiceAPI/LynxServiceLogProtocol.h>  // nogncheck
#import <LynxServiceAPI/ServiceAPI.h>              // nogncheck
#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging_darwin.h"
#include "base/src/base_trace/base_trace_event_def.h"
#include "base/src/base_trace/trace_event.h"

#define LOCKED(...)             \
  @synchronized(gDelegateDic) { \
    __VA_ARGS__;                \
  }

@implementation LynxLogDelegate

- (instancetype)initWithLogFunction:(LynxLogFunction)logFunction
                        minLogLevel:(LynxLogLevel)minLogLevel {
  if (self = [super init]) {
    self.logFunction = logFunction;
    self.minLogLevel = minLogLevel;
    self.acceptSource = NSIntegerMax;
    self.acceptRuntimeId = -1;
    self.shouldFormatMessage = true;
  }
  return self;
}

@end

static NSInteger gDefaultDelegateId = -1;
static NSInteger gCurrentId = 0;
static NSMutableDictionary *gDelegateDic = [[NSMutableDictionary alloc] init];
#ifdef DEBUG
static LynxLogLevel gLogMinLevel = LynxLogLevelDebug;
#else
static LynxLogLevel gLogMinLevel = LynxLogLevelInfo;
#endif
static bool gIsJSLogsFromExternalChannelsOpen = false;
static LynxLogDelegate *gDebugLoggingDelegate;

void SetDebugLoggingDelegate(LynxLogDelegate *delegate) { gDebugLoggingDelegate = delegate; }

void PrintLogMessageForDebug(LynxLogLevel level, NSString *message) {
  if (gDebugLoggingDelegate == nullptr || level < gDebugLoggingDelegate.minLogLevel) {
    return;
  }
  LynxLogFunction logFunction = gDebugLoggingDelegate.logFunction;
  if (logFunction == nil) {
    return;
  }
  logFunction(level, message);
}

// turn off by default
// JS logs form external channels: recorded by business developers (mostly front-end)
void SetJSLogsFromExternalChannels(bool isOpen) { gIsJSLogsFromExternalChannelsOpen = isOpen; }

namespace lynx {
namespace base {
namespace logging {

bool IsExternalChannel(lynx::base::logging::LogChannel channelType) {
  return gIsJSLogsFromExternalChannelsOpen &&
         channelType == lynx::base::logging::LOG_CHANNEL_LYNX_EXTERNAL;
}

void PrintLogMessageByLogDelegate(LogMessage *msg, const char *tag) {
  LynxLogLevel level = (LynxLogLevel)msg->severity();
  NSString *message = gDebugLoggingDelegate.shouldFormatMessage
                          ? [NSString stringWithUTF8String:msg->stream().str().c_str()]
                          : [[NSString stringWithUTF8String:msg->stream().str().c_str()]
                                substringFromIndex:msg->messageStart()];
  // print native's log to devtool for debug
  PrintLogMessageForDebug(level, message);

  NSArray<LynxLogDelegate *> *delegates = GetLoggingDelegates();
  for (LynxLogDelegate *delegate in delegates) {
    LynxLogFunction logFunction = delegate.logFunction;
    if (logFunction == nil || level < gLogMinLevel ||
        (delegate.acceptRuntimeId >= 0 && delegate.acceptRuntimeId != msg->runtimeId())) {
      continue;
    }
    message = delegate.shouldFormatMessage
                  ? [NSString stringWithUTF8String:msg->stream().str().c_str()]
                  : [[NSString stringWithUTF8String:msg->stream().str().c_str()]
                        substringFromIndex:msg->messageStart()];
    if (message == nil) {
      return;
    }
    // only upload external JS logs and console.report to logging delegate
    switch (msg->source()) {
      case LOG_SOURCE_JS:
        if (IsExternalChannel(msg->ChannelType()) && (delegate.acceptSource & LynxLogSourceJS)) {
          logFunction(level, message);
        }
        break;
      case LOG_SOURCE_JS_EXT:
        logFunction(level, message);
        break;
#if OS_MAC
      // output the native log of lynx when alog is not supported on the PC(Windows & Mac)
      case LOG_SOURCE_NATIVE:
        logFunction(level, message);
        break;
#endif
      default:
        break;
    }
  }
}

}  // namespace logging
}  // namespace base
}  // namespace lynx

void InitLynxLog(bool print_logs_to_all_channels) {
  id service = LynxService(LynxServiceLogProtocol);
  void *log_address = nullptr;
  if (service) {
    log_address = (void *)[service getWriteFunction];
  }
  lynx::base::logging::InitLynxLoggingNative(
      log_address, lynx::base::logging::PrintLogMessageByLogDelegate, print_logs_to_all_channels);
}

NSInteger AddLoggingDelegate(LynxLogDelegate *delegate) {
  NSInteger delegateId = ++gCurrentId;
  LOCKED([gDelegateDic setObject:delegate forKey:@(delegateId)]);
  return delegateId;
}

LynxLogDelegate *GetLoggingDelegate(NSInteger delegateId) {
  LOCKED(return [gDelegateDic objectForKey:@(delegateId)]);
}

void RemoveLoggingDelegate(NSInteger delegateId) {
  LOCKED([gDelegateDic removeObjectForKey:@(delegateId)]);
}

NSArray<LynxLogDelegate *> *GetLoggingDelegates(void) { LOCKED(return [gDelegateDic allValues]); }

void SetMinimumLoggingLevel(LynxLogLevel minLogLevel) {
  [[maybe_unused]] static constexpr const char *kLogLevelName[] = {
      "LynxLogLevelVerbose", "LynxLogLevelDebug", "LynxLogLevelInfo",
      "LynxLogLevelWarning", "LynxLogLevelError", "LynxLogLevelFatal"};
  if (gLogMinLevel < minLogLevel) {
    gLogMinLevel = minLogLevel;
    lynx::base::logging::SetLynxLogMinLevel(static_cast<int>(minLogLevel));
    NSLog(@"W/lynx: Reset minimum log level as %s", kLogLevelName[gLogMinLevel]);
  } else {
    NSLog(@"W/lynx: Please set a log level higher than %s to filter lynx logs!",
          kLogLevelName[gLogMinLevel]);
  }
}

LynxLogLevel GetMinimumLoggingLevel(void) { return gLogMinLevel; }

NSInteger LynxSetLogFunction(LynxLogFunction logFunction) {
  LynxLogDelegate *delegate = [[LynxLogDelegate alloc] initWithLogFunction:logFunction
                                                               minLogLevel:LynxLogLevelInfo];
  gDefaultDelegateId = AddLoggingDelegate(delegate);
  return gDefaultDelegateId;
}

LynxLogFunction LynxGetLogFunction(void) {
  LynxLogDelegate *delegate = GetLoggingDelegate(gDefaultDelegateId);
  if (!delegate) {
    return LynxDefaultLogFunction;
  }
  return delegate.logFunction;
}

LynxLogFunction LynxDefaultLogFunction = ^(LynxLogLevel level, NSString *message) {
  NSLog(@"%s/lynx: %@", lynx::base::logging::kLynxLogLevels[level], message);
};

void _LynxLogInternal(const char *file, int32_t line, LynxLogLevel level, NSString *format, ...) {
  BASE_TRACE_EVENT(LYNX_BASE_TRACE_CATEGORY, LYNX_BASE_LOG_INTERNAL);
  @autoreleasepool {
    va_list args;
    va_start(args, format);
    NSString *messageTail = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    NSString *message = [[NSString alloc] initWithFormat:@"[%s:%s(%d)] %@",
                                                         lynx::base::logging::kLynxLogLevels[level],
                                                         file, line, messageTail];
    // print log
    if (level >= gLogMinLevel) {
      lynx::base::logging::InternalLogNative(file, line, (int)level, [message UTF8String]);
    }

    // print log to devtool for debug
    PrintLogMessageForDebug(level, message);
  }
}

void _LynxErrorInfo(NSInteger errorCode, NSString *format, ...) {
  va_list args;
  va_start(args, format);
  NSString *message = [[NSString alloc] initWithFormat:format arguments:args];
  va_end(args);
  LynxInfo(static_cast<int>(errorCode), message.UTF8String);
}

void _LynxErrorWarning(bool expression, NSInteger errorCode, NSString *format, ...) {
  if (!expression) {
    va_list args;
    va_start(args, format);
    NSString *message = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    LynxWarning(expression, static_cast<int>(errorCode), message.UTF8String);
  }
}

void _LynxErrorFatal(bool expression, NSInteger errorCode, NSString *format, ...) {
  if (!expression) {
    va_list args;
    va_start(args, format);
    NSString *message = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    LynxFatal(expression, static_cast<int>(errorCode), message.UTF8String);
  }
}

const char *_GetLastPath(const char *filename, int32_t length) {
  return lynx::base::PathUtils::GetLastPath(filename, length);
}
