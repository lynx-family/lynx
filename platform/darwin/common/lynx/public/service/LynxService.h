// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICE_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICE_H_
#import <Foundation/Foundation.h>
#import "LynxDefines.h"
#import "LynxServiceModuleProtocol.h"
#import "LynxServiceMonitorProtocol.h"
#import "LynxServiceProtocol.h"
#import "LynxServiceSecurityProtocol.h"
#import "LynxServiceTrailProtocol.h"
#if TARGET_OS_IOS
#import "LynxServiceImageProtocol.h"
#endif

NS_ASSUME_NONNULL_BEGIN

/*
 * You can use @LynxServiceRegister to specify a LynxService will be
 * registered into LynxServices instance automatically. This annotation
 * should only be used for LynxServices subclasses, and before the
 * @implementation code fragment in the .m file. e.g.: LynxMonitorService
 * is a subclass of LynxServices, in LynxMonitorService.m
 * // ...import headers...
 * @LynxServiceRegister(LynxMonitorService)
 * @implmentation LynxMonitorService
 * //...
 * @end
 */

#ifndef LynxServiceRegister
#define LynxServiceRegister(clsName)                                                   \
  interface LynxServices(clsName) @end @implementation LynxServices(clsName)           \
  +(NSString *)LYNX_CONCAT(__lynx_auto_register_serivce__,                             \
                           LYNX_CONCAT(clsName, LYNX_CONCAT(__LINE__, __COUNTER__))) { \
    return @ #clsName;                                                                 \
  }                                                                                    \
  @end
#endif

/**
 * Bind protocol and class, e.g., LYNX_SERVICE_BIND (LynxMonitorService,
 * LynxMonitorProtocol)
 */
#define LynxServiceBind(cls, pro) ([LynxServices bindClass:cls toProtocol:@protocol(pro)])

/**
 * Get the default object that implements the specified protocol, e.g.,
 * LYNX_SERVICE(LynxMonitorProtocol) -> id<LynxMonitorProtocol>
 */
#define LynxService(pro) \
  ((id<pro>)([LynxServices getInstanceWithProtocol:@protocol(pro) bizID:DEFAULT_LYNX_SERVICE]))
/**
 * Get the object that implements the specified protocol, e.g.,
 * LYNX_SERVICE(LynxMonitorProtocol, default) -> id<LynxMonitorProtocol>
 */
#define LynxServiceBID(pro, bid) \
  ((id<pro>)([LynxServices getInstanceWithProtocol:@protocol(pro) bizID:bid]))

/**
 * Get LynxTrail Instance
 * LynxTrail
 */
#define LynxTrail LynxService(LynxServiceTrailProtocol)

@interface LynxServices : NSObject

/**
 * Register default service
 * @param cls Protocol implementation class, which implements sharedInstance and is a singleton.
 */
+ (void)registerService:(Class)cls;

/**
 * Get implementation through protocol
 * @param protocol The protocol, which implements sharedInstance and returns a singleton
 * @param bizID Business ID
 * @return An instance of the protocol implementation class; returns null if the protocol has not
 * been bound
 */
+ (id)getInstanceWithProtocol:(Protocol *)protocol bizID:(NSString *__nullable)bizID;

@end

NS_ASSUME_NONNULL_END
#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICE_H_
