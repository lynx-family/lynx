// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCEREQUESTOPERATIONPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCEREQUESTOPERATIONPROTOCOL_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceResourceRequestOperationProtocol <NSObject>

/**
 * The url of the request.
 */
@property(nonatomic, copy) NSString* _Nullable url;

/**
 * Cancel the request.
 */
- (BOOL)cancel;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICERESOURCEREQUESTOPERATIONPROTOCOL_H_
