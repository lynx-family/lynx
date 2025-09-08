// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxLog.h>

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

NSArray<LynxLogObserver *> *LynxGetLogObservers() { return GetLoggingDelegates(); }

void LynxSetMinLogLevel(LynxLogLevel minLogLevel) { SetMinimumLoggingLevel(minLogLevel); }

LynxLogLevel LynxGetMinLogLevel(void) { return GetMinimumLoggingLevel(); }
