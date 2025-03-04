// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLog.h"
#include <map>
#import "LynxEnv.h"
#import "LynxTraceEvent.h"

#include "base/include/debug/lynx_assert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/darwin/logging_darwin.h"
#include "core/base/lynx_trace_categories.h"

#define LOCKED(...)             \
  @synchronized(gDelegateDic) { \
    __VA_ARGS__;                \
  }

@implementation LynxLogObserver

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
static NSInteger gDefaultRuntimeId = -1;
static NSMutableDictionary *gDelegateDic = [[NSMutableDictionary alloc] init];
#ifdef DEBUG
static LynxLogLevel gLogMinLevel = LynxLogLevelDebug;
#else
static LynxLogLevel gLogMinLevel = LynxLogLevelInfo;
#endif
static bool gIsJSLogsFromExternalChannelsOpen = false;
static LynxLogDelegate *gDebugLoggingDelegate;

void SetDebugLoggingDelegate(LynxLogDelegate *delegate) { gDebugLoggingDelegate = delegate; }

void PrintLogMessageForDebug(LynxLogLevel level, NSString *message,
                             int64_t runtimeId = gDefaultRuntimeId) {
  if (gDebugLoggingDelegate == nullptr || level < gDebugLoggingDelegate.minLogLevel) {
    return;
  }
  LynxLogFunction logFunction = gDebugLoggingDelegate.logFunction;
  if (logFunction == nil) {
    return;
  }
  NSString *msgWithRid = message;
  if (runtimeId != gDefaultRuntimeId) {
    msgWithRid = [NSString stringWithFormat:@"argRuntimeId:%lld&%@", runtimeId, message];
  }
  logFunction(level, msgWithRid);
}

// turn off by default
// JS logs form external channels: recorded by business developers (mostly front-end)
void SetJSLogsFromExternalChannels(bool isOpen) { gIsJSLogsFromExternalChannelsOpen = isOpen; }

namespace lynx {
namespace base {
namespace logging {
namespace {
NSArray<LynxLogDelegate *> *GetLoggingDelegates(void) { LOCKED(return [gDelegateDic allValues]); }
}  // namespace

bool IsExternalChannel(lynx::base::logging::LogChannel channelType) {
  return gIsJSLogsFromExternalChannelsOpen &&
         channelType == lynx::base::logging::LOG_CHANNEL_LYNX_EXTERNAL;
}

// Implementation of this function in the <base/Darwin/logging_darwin.h> file.
void PrintLogMessageByLogDelegate(LogMessage *msg, const char *tag) {
  LynxLogLevel level = (LynxLogLevel)msg->severity();
  NSString *message = gDebugLoggingDelegate.shouldFormatMessage
                          ? [NSString stringWithUTF8String:msg->stream().str().c_str()]
                          : [[NSString stringWithUTF8String:msg->stream().str().c_str()]
                                substringFromIndex:msg->messageStart()];
  // print native's log to devtool for debug
  PrintLogMessageForDebug(level, message, msg->runtimeId());

  NSArray<LynxLogDelegate *> *delegates = GetLoggingDelegates();
  for (LynxLogDelegate *delegate in delegates) {
    LynxLogFunction logFunction = delegate.logFunction;
    if (logFunction == nil || level < delegate.minLogLevel ||
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

void InitLynxLog(bool enable_devtools) {
  lynx::base::logging::InitLynxLoggingNative(
      enable_devtools);  // tasm::LynxEnv::GetInstance().IsDevtoolEnabled());
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

NSInteger LynxAddLogObserver(LynxLogFunction logFunction, LynxLogLevel minLogLevel) {
  LynxLogDelegate *delegate = [[LynxLogDelegate alloc] initWithLogFunction:logFunction
                                                               minLogLevel:minLogLevel];
  return AddLoggingDelegate(delegate);
}

NSInteger LynxAddLogObserverByModel(LynxLogObserver *observer) {
  return AddLoggingDelegate(observer);
}

LynxLogObserver *LynxGetLogObserver(NSInteger observerId) { return GetLoggingDelegate(observerId); }

void LynxRemoveLogObserver(NSInteger observerId) { RemoveLoggingDelegate(observerId); }

NSArray<LynxLogObserver *> *LynxGetLogObservers() { LOCKED(return [gDelegateDic allValues]); }

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

void LynxSetMinLogLevel(LynxLogLevel minLogLevel) {
  LynxLogDelegate *delegate = GetLoggingDelegate(gDefaultDelegateId);
  if (delegate) {
    delegate.minLogLevel = minLogLevel;
  }
  SetMinimumLoggingLevel(minLogLevel);
}

LynxLogLevel LynxGetMinLogLevel(void) {
  LynxLogDelegate *delegate = GetLoggingDelegate(gDefaultDelegateId);
  if (delegate) {
    return delegate.minLogLevel;
  }
  return LynxLogLevelInfo;
}

LynxLogFunction LynxDefaultLogFunction = ^(LynxLogLevel level, NSString *message) {
  NSLog(@"%s/lynx: %@", lynx::base::logging::kLynxLogLevels[level], message);
};

void _LynxLogInternal(const char *file, int32_t line, LynxLogLevel level, NSString *format, ...) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "_LynxLogInternal");
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
