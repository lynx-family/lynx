// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEPROTOCOL_H_

#import <Foundation/Foundation.h>

#define DEFAULT_LYNX_SERVICE @"lynx_default_service"

static NSUInteger const kLynxServiceTypeMonitor = 1;
static NSUInteger const kLynxServiceHttp = 2;
static NSUInteger const kLynxServiceTrail = 3;
static NSUInteger const kLynxServiceImage = 4;
static NSUInteger const kLynxServiceEventReporter = 6;
static NSUInteger const kLynxServiceModule = 7;
static NSUInteger const kLynxServiceLog = 8;
static NSUInteger const kLynxServiceI18n = 9;
static NSUInteger const kLynxServiceSystemInvoke = 10;
static NSUInteger const kLynxServiceResource = 11;
static NSUInteger const kLynxServiceSecurity = 12;
static NSUInteger const kLynxServiceDevTool = 13;
static NSUInteger const kLynxServiceExtension = 14;

typedef NS_OPTIONS(NSUInteger, LynxServiceScope) {
  LynxServiceScopeDefault = 1 << 0,
  LynxServiceScopeBiz = 1 << 1
};

@protocol LynxServiceProtocol <NSObject>

@required
/// Service Scope type
+ (LynxServiceScope)serviceScope;

/// The type of current service
+ (NSUInteger)serviceType;

/// The biz tag of current service.
+ (NSString *)serviceBizID;

@optional
+ (instancetype)sharedInstance;

@end

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEPROTOCOL_H_
