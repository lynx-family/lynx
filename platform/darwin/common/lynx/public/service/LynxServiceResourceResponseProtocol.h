// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCERESPONSEPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCERESPONSEPROTOCOL_H_

#import <Foundation/Foundation.h>
#import "LynxResourceResponseDataInfoProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceResourceResponseProtocol <LynxResourceResponseDataInfoProtocol>

// The content of resource
- (nullable NSData *)data;

// is success
- (BOOL)isSuccess;

@end

/**
 * The completion handler for fetch resource request.
 */
typedef void (^LynxServiceResourceCompletionHandler)(
    id<LynxServiceResourceResponseProtocol> __nullable response, NSError *__nullable error);

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCERESPONSEPROTOCOL_H_
