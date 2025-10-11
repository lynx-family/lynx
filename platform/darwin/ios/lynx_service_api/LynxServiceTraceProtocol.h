// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_SERVICE_API_LYNX_SERVICE_TRACE_PROTOCOL_H_
#define DARWIN_SERVICE_API_LYNX_SERVICE_TRACE_PROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxServiceAPI/LynxServiceProtocol.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceTraceProtocol <LynxServiceProtocol>

- (void *)getDefaultTraceFunction;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_SERVICE_API_LYNX_SERVICE_TRACE_PROTOCOL_H_
