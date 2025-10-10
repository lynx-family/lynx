// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_SERVICE_API_LYNX_SERVICE_LOG_PROTOCOL_H_
#define DARWIN_SERVICE_API_LYNX_SERVICE_LOG_PROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxServiceAPI/ServiceAPI.h>

@protocol LynxServiceLogProtocol <LynxServiceProtocol>

- (void *)getWriteFunction;

@end

#endif /* DARWIN_SERVICE_API_LYNX_SERVICE_LOG_PROTOCOL_H_ */
