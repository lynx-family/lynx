// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICE_H_
#define BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICE_H_

#import <Foundation/Foundation.h>
#import <LynxBase/LynxBaseServiceProtocol.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Register default service.
 */
#define RegisterLynxBaseService(cls) [LynxBaseServices registerService:cls]

/**
 * Get the default object that implements the specified protocol, e.g.,
 * LynxBaseService(LynxBaseServiceLogProtocol) -> id<LynxBaseServiceLogProtocol>
 */
#define LynxBaseService(pro)                                          \
  ((id<pro>)([LynxBaseServices getInstanceWithProtocol:@protocol(pro) \
                                                 bizID:DEFAULT_LYNX_BASE_SERVICE]))

@interface LynxBaseServices : NSObject

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
#endif  // BASE_PLATFORM_DARWIN_SERVICE_LYNXBASESERVICE_H_
