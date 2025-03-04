// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLog.h"

#ifndef LYNX_LIST_DEBUG
#define LYNX_LIST_DEBUG 0
#define LYNX_LIST_DEBUG_LABEL 0
#if LYNX_LIST_DEBUG
#define LYNX_LIST_DEBUG_LOG(FORMAT, ...) LYNX_DEBUG_LOG(LynxListDebug, FORMAT, ##__VA_ARGS__)

#define LYNX_LIST_DEBUG_COND_LOG(COND, FORMAT, ...) \
  do {                                              \
    if (COND) {                                     \
      LYNX_LIST_DEBUG_LOG(FORMAT, ##__VA_ARGS__)    \
    }                                               \
  } while (0)

#else
#define LYNX_LIST_DEBUG_LOG(FORMAT, ...)
#define LYNX_LIST_DEBUG_COND_LOG(COND, FORMAT, ...)
#endif  // #if LYNX_LIST_DEBUG
#endif  // #ifndef LYNX_LIST_DEBUG

typedef NS_ENUM(NSInteger, LynxListDebugInfoLevel) {
  LynxListDebugInfoLevelNone,
  LynxListDebugInfoLevelError,
  LynxListDebugInfoLevelInfo,
  LynxListDebugInfoLevelVerbose,
};
