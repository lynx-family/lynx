// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICE_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICE_H_

#import <Foundation/Foundation.h>
#import <Lynx/LynxServiceModuleProtocol.h>
#import <Lynx/LynxServiceMonitorProtocol.h>
#import <Lynx/LynxServiceSecurityProtocol.h>
#import <LynxServiceAPI/LynxServiceTrailProtocol.h>
#import <LynxServiceAPI/ServiceAPI.h>

#if TARGET_OS_IOS
#import <Lynx/LynxServiceImageProtocol.h>
#endif

#import <Lynx/LynxServiceTrailExtensionProtocol.h>

@interface LynxServiceHelper : NSObject
+ (nullable id<LynxServiceTrailExtensionProtocol>)getLynxTrailExtensionService;
@end

/**
 * Get LynxTrailService Instance
 */
#define LynxTrail LynxService(LynxServiceTrailProtocol)
/**
 * Get extended LynxTrailService Instance
 */
#define LynxTrailExtensionService ([LynxServiceHelper getLynxTrailExtensionService])

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICE_H_
