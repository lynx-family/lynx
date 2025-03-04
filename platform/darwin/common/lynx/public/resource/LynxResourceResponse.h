// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCERESPONSE_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCERESPONSE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
// default code for resource provider
FOUNDATION_EXPORT NSInteger const LynxResourceResponseCodeSuccess;
FOUNDATION_EXPORT NSInteger const LynxResourceResponseCodeFailed;

/**
 *Lynx Standard Resource Request
 */
@interface LynxResourceResponse : NSObject

@property(nonatomic, readonly, strong) id data;

@property(nonatomic, readonly, strong) NSError* error;

@property(nonatomic, readonly, assign) NSInteger code;

- (instancetype)initWithData:(id)data;

- (instancetype)initWithError:(NSError*)error code:(NSInteger)code;

- (bool)success;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXRESOURCERESPONSE_H_
