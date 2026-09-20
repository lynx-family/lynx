// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_LOG_LYNXLOG_PRIVATE_H_
#define BASE_PLATFORM_DARWIN_LOG_LYNXLOG_PRIVATE_H_

#import <LynxBase/LynxLog.h>

// NOTE: 0 is reserved for original INFO; future INFO sublevels must be nonzero.
typedef NS_ENUM(NSInteger, LynxInfoLogLevel) {
  LynxInfoLogLevelMonitor = 5,
  LynxInfoLogLevelObserve = 10,
  LynxInfoLogLevelInfo = 15
};

LYNX_BASE_EXTERN NSInteger GetInfoLoggingLevel(void);

// TODO: Preserve M/O identity if output consumers need it in the future.
#define LLogMonitor(...)                                    \
  do {                                                      \
    if (GetInfoLoggingLevel() <= LynxInfoLogLevelMonitor) { \
      LLogInfo(__VA_ARGS__);                                \
    }                                                       \
  } while (0)
#define LLogObserve(...)                                    \
  do {                                                      \
    if (GetInfoLoggingLevel() <= LynxInfoLogLevelObserve) { \
      LLogInfo(__VA_ARGS__);                                \
    }                                                       \
  } while (0)
#define _LogM(...) LLogMonitor(__VA_ARGS__)
#define _LogO(...) LLogObserve(__VA_ARGS__)

#endif  // BASE_PLATFORM_DARWIN_LOG_LYNXLOG_PRIVATE_H_
