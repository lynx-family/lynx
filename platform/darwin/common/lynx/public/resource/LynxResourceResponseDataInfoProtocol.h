// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNX_RESOURCE_RESPONSE_DATA_INFO_PROTOCOL_H_
#define LYNX_RESOURCE_RESPONSE_DATA_INFO_PROTOCOL_H_
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxResourceResponseDataInfoProtocol <NSObject>

/**
 * The data of the response.
 */
- (nullable NSData *)data;

/**
 * The response status.
 */
- (BOOL)isSuccess;

@end

NS_ASSUME_NONNULL_END

#endif  // LYNX_RESOURCE_RESPONSE_DATA_INFO_PROTOCOL_H_
