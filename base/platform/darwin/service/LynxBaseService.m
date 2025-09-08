// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <LynxBase/LynxBaseService.h>
#import <LynxBase/LynxBaseServiceLogProtocol.h>
#import <LynxBase/LynxBaseServiceTraceProtocol.h>
#import <LynxBase/LynxLog.h>

#import <objc/runtime.h>

@interface LynxBaseServices ()

@property(atomic, strong) NSMutableDictionary<NSString *, Class> *protocolToClassMap;
@property(atomic, strong) NSMutableDictionary<NSString *, id> *protocolToInstanceMap;
@property(atomic, strong) NSRecursiveLock *recLock;

@end

@implementation LynxBaseServices

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static LynxBaseServices *services;
  dispatch_once(&onceToken, ^{
    services = [[LynxBaseServices alloc] init];
  });
  return services;
}

+ (void)registerService:(Class)cls {
  if (cls == nil || ![cls conformsToProtocol:@protocol(LynxBaseServiceProtocol)]) {
    NSAssert(NO, @"%@ should conforms to %@, %s", NSStringFromClass(cls),
             NSStringFromProtocol(@protocol(LynxBaseServiceProtocol)), __func__);
    return;
  }
  Protocol *protocol = [self getProtocolByServiceType:[cls getServiceType]];
  if (protocol != nil) {
    [[self sharedInstance] bindClass:cls toProtocol:protocol];
  } else {
    LLogInfo(@"Unknow lynx service type - %lu", [cls getServiceType]);
  }
}

+ (Protocol *)getProtocolByServiceType:(NSUInteger)serviceType {
  switch (serviceType) {
    case kLynxBaseServiceLog:
      return @protocol(LynxBaseServiceLogProtocol);
    case kLynxBaseServiceTrace:
      return @protocol(LynxBaseServiceTraceProtocol);
    default:
      return nil;
  }
  return nil;
}

#pragma mark - Lifecycle
- (instancetype)init {
  if (self = [super init]) {
    _protocolToClassMap = [[NSMutableDictionary alloc] init];
    _protocolToInstanceMap = [[NSMutableDictionary alloc] init];
    _recLock = [[NSRecursiveLock alloc] init];
  }
  return self;
}

#pragma mark - Public

+ (void)bindClass:(Class)cls toProtocol:(Protocol *)protocol {
  [[self sharedInstance] bindClass:cls toProtocol:protocol];
}

+ (id)getInstanceWithProtocol:(Protocol *)protocol bizID:bid {
  if (!protocol || protocol == nil) {
    return nil;
  }
  return [[self sharedInstance] getInstanceWithProtocol:protocol bizID:bid];
}

- (void)bindClass:(Class)cls toProtocol:(Protocol *)protocol {
  if (cls == nil || protocol == nil || ![cls conformsToProtocol:protocol]) {
    NSAssert(NO, @"%@ should conforms to %@, %s", NSStringFromClass(cls),
             NSStringFromProtocol(protocol), __func__);
    return;
  }
  if (![cls conformsToProtocol:@protocol(LynxBaseServiceProtocol)]) {
    NSAssert(NO, @"%@ should conforms to LynxBaseServiceProtocol, %s", NSStringFromClass(cls),
             __func__);
    return;
  }
  [self.recLock lock];
  // Get biz name from clz instance
  // key: protocolName__lynx_base_binder__${bizID}
  NSString *protocolName =
      [NSString stringWithFormat:@"%@__lynx_base_binder__%@", NSStringFromProtocol(protocol),
                                 [cls getServiceBizID] ?: DEFAULT_LYNX_BASE_SERVICE];
  if ([self.protocolToClassMap objectForKey:protocolName] == nil) {
    [self.protocolToClassMap setObject:cls forKey:protocolName];
  } else {
    NSAssert(NO, @"%@ and %@ are duplicated bindings, %s", NSStringFromClass(cls),
             NSStringFromProtocol(protocol), __func__);
  }
  [self.recLock unlock];
}

- (id)getInstanceWithProtocol:(Protocol *)protocol bizID:(NSString *)bizID {
  [self.recLock lock];
  // key: protocolName__lynx_base_binder__${bizID}
  NSString *finalBizID = bizID ?: DEFAULT_LYNX_BASE_SERVICE;
  NSString *protocolName = [NSString
      stringWithFormat:@"%@__lynx_base_binder__%@", NSStringFromProtocol(protocol), finalBizID];
  id object = [self.protocolToInstanceMap objectForKey:protocolName];
  if (object == nil) {
    Class cls = [self.protocolToClassMap objectForKey:protocolName];
    if (cls != nil) {
      object =
          [self.protocolToInstanceMap objectForKey:protocolName] ?: [self getInstanceWithClass:cls];
      NSAssert(object, @"%@ no object", protocolName);
      if (object) {
        [self.protocolToInstanceMap setObject:object forKey:protocolName];
      }
    } else {
      // NSAssert(NO, @"%@ is not binded, %s", NSStringFromProtocol(protocol), __func__);
    }
  }
  [self.recLock unlock];
  return object;
}

- (id)getInstanceWithClass:(Class)cls {
  // UI types need to check the main thread
  id object = nil;
  if ([cls respondsToSelector:@selector(sharedInstance)]) {
    object = [cls sharedInstance];
  } else {
    object = [[cls alloc] init];
  }
  return object;
}

@end
