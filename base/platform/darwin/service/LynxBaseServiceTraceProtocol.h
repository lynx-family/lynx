// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICETRACEPROTOCOL_H_
#define BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICETRACEPROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxBase/LynxBaseServiceProtocol.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, TraceEventType) {
  kTypeUnspecified,
  kTypeSliceBegin,
  kTypeSliceEnd,
  kTypeInstant,
  kTypeCounter,
};

@protocol LynxBaseServiceTraceProtocol <LynxBaseServiceProtocol>

- (void)trace:(const char*)category name:(const char*)name phase:(TraceEventType)phase;

@end

NS_ASSUME_NONNULL_END

#endif  // BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICETRACEPROTOCOL_H_
