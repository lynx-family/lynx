// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCEPROVIDER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCEPROVIDER_H_

#import <Foundation/Foundation.h>

#import "LynxResourceRequest.h"
#import "LynxResourceResponse.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxResourceLoadBlock)(LynxResourceResponse* response);
/**
 *Standard Lynx Resource Provider,  any resource provider should implement it
 */
@protocol LynxResourceProvider <NSObject>

/**
 * for resoure request, must be overrode
 * param request:   request object, contain url and params
 * param callback:  callback when resource finish
 */
- (void)request:(LynxResourceRequest*)request onComplete:(LynxResourceLoadBlock)callback;

/**
 * Cancel request which has requested, default empty body. Called from Lynx
 * param request:  witch will be cancelled
 */
- (void)cancel:(LynxResourceRequest*)request;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCEPROVIDER_H_
