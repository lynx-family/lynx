// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_SERVICE_API_SERVICE_API_H_
#define DARWIN_SERVICE_API_SERVICE_API_H_
#import <Foundation/Foundation.h>
#import <LynxServiceAPI/LynxServiceProtocol.h>
#import <LynxServiceAPI/ServiceLazyLoad.h>

NS_ASSUME_NONNULL_BEGIN

#define STRINGIFY(x) #x
#define STRINGIFY_(x) STRINGIFY(x)

// Assert that the SubProtocol inherits the SuperProtocol
#define REQUIRE_PROTOCOL_INHERITANCE(SubProtocol, SuperProtocol)                   \
  do {                                                                             \
    __unused id<SuperProtocol> _protocol_inheritance_check = (id<SubProtocol>)nil; \
  } while (0)

// Assert that the Class implements the Protocol
#define REQUIRE_CLASS_PROTOCOL(Class, Protocol)                 \
  do {                                                          \
    __unused id<Protocol> _class_protocol_check = (Class *)nil; \
  } while (0)

typedef struct {
  Class classObj;
  Protocol *protocolObj;
} LynxServiceEntry;

#define LYNX_AUTO_REGISTER_SERVICE_PREFIX __lynx_auto_register_service__

#ifndef LYNX_CONCAT
#define LYNX_CONCAT2(A, B) A##B
#define LYNX_CONCAT(A, B) LYNX_CONCAT2(A, B)
#endif

/*
 * You can use @LynxServiceRegister to specify a LynxService will be
 * registered into LynxServices instance automatically. This annotation
 * should only be used for LynxServices subclasses, and before the
 * @implementation code fragment in the .m file. e.g.: LynxMonitorService
 * is a subclass of LynxServices, in LynxMonitorService.m
 * // ...import headers...
 * @LynxServiceRegister(LynxMonitorService, LynxServiceMonitorProtocol)
 * @implementation LynxMonitorService
 * //...
 * @end
 */
#ifndef LynxServiceRegister
#define LynxServiceRegister(clsName, protocolName)                                             \
  interface LynxServices(clsName) @end @implementation LynxServices(clsName)                   \
  +(LynxServiceEntry *)LYNX_CONCAT(LYNX_AUTO_REGISTER_SERVICE_PREFIX,                          \
                                   LYNX_CONCAT(clsName, LYNX_CONCAT(__LINE__, __COUNTER__))) { \
    REQUIRE_CLASS_PROTOCOL(clsName, protocolName);                                             \
    REQUIRE_PROTOCOL_INHERITANCE(protocolName, LynxServiceProtocol);                           \
    static LynxServiceEntry entry = {.classObj = NULL, .protocolObj = NULL};                   \
    if (entry.classObj == NULL) {                                                              \
      entry.classObj = [clsName class];                                                        \
      entry.protocolObj = @protocol(protocolName);                                             \
    }                                                                                          \
    return &entry;                                                                             \
  }                                                                                            \
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
#define LynxService(pro) ((id<pro>)([LynxServices getInstanceWithProtocol:@protocol(pro)]))

@interface LynxServices : NSObject

/**
 * Register service implementation for protocol
 */
+ (void)registerServiceWithProtocol:(Class)cls protocol:(Protocol *)proto;

/**
 * Get implementation through protocol
 * @param protocol The protocol, which implements sharedInstance and returns a singleton
 * @return An instance of the protocol implementation class; returns null if the protocol has not
 * been bound
 */
+ (id)getInstanceWithProtocol:(Protocol *)protocol;

@end

NS_ASSUME_NONNULL_END
#endif  // DARWIN_SERVICE_API_SERVICE_API_H_
