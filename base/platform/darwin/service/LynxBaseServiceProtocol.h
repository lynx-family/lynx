// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICEPROTOCOL_H_
#define BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICEPROTOCOL_H_

#import <Foundation/Foundation.h>

#define DEFAULT_LYNX_BASE_SERVICE @"lynx_base_default_service"

static NSUInteger const kLynxBaseServiceLog = 1;
static NSUInteger const kLynxBaseServiceTrace = 2;

@protocol LynxBaseServiceProtocol <NSObject>

/// The type of current service
+ (NSUInteger)getServiceType;

/// The biz tag of current service.
+ (NSString *)getServiceBizID;

@optional
+ (instancetype)sharedInstance;

@end

#endif  // BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICEPROTOCOL_H_
