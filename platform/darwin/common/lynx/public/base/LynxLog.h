// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXLOG_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXLOG_H_

#import <Lynx/LynxDefines.h>
#import <LynxBase/LynxLog.h>

// LynxLogDelegate is recommended and LynxLogObserver is prohibited.
typedef LynxLogDelegate LynxLogObserver;

// deprecated: use AddLoggingDelegate instead.
LYNX_EXTERN NSInteger LynxAddLogObserver(LynxLogFunction logFunction, LynxLogLevel minLogLevel);
// deprecated: use AddLoggingDelegate instead.
LYNX_EXTERN NSInteger LynxAddLogObserverByModel(LynxLogObserver *observer);
// deprecated: use GetLoggingDelegate instead.
LYNX_EXTERN LynxLogObserver *LynxGetLogObserver(NSInteger observerId);
// deprecated: use RemoveLoggingDelegate instead.
LYNX_EXTERN void LynxRemoveLogObserver(NSInteger observerId);
LYNX_EXTERN NSArray<LynxLogObserver *> *LynxGetLogObservers(void);
LYNX_EXTERN void LynxSetMinLogLevel(LynxLogLevel minLogLevel)
    __attribute__((deprecated("Use SetMinimumLoggingLevel instead.")));
// deprecated: use GetMinimumLoggingLevel instead.
LYNX_EXTERN LynxLogLevel LynxGetMinLogLevel(void);
#endif  // DARWIN_COMMON_LYNX_BASE_LYNXLOG_H_
