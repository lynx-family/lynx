// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_SERVICE_API_LYNX_SERVICE_PROTOCOL_H_
#define DARWIN_SERVICE_API_LYNX_SERVICE_PROTOCOL_H_

#import <Foundation/Foundation.h>

#define DEFAULT_LYNX_SERVICE @"lynx_default_service"

typedef NS_OPTIONS(NSUInteger, LynxServiceScope) {
  LynxServiceScopeDefault = 1 << 0,
  LynxServiceScopeBiz = 1 << 1
};

@protocol LynxServiceProtocol <NSObject>

@required

@optional
/// Service Scope type
+ (LynxServiceScope)serviceScope __attribute__((
    deprecated("This method is deprecated and will be removed in a future release")));

/// The type of current service
+ (NSUInteger)serviceType __attribute__((
    deprecated("This method is deprecated and will be removed in a future release")));

/// The biz tag of current service.
+ (NSString *)serviceBizID __attribute__((
    deprecated("This method is deprecated and will be removed in a future release")));

+ (instancetype)sharedInstance;

@end

#endif  // DARWIN_SERVICE_API_LYNX_SERVICE_PROTOCOL_H_
