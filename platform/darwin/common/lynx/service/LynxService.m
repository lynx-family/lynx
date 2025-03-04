// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxService.h"
#import "LynxLazyLoad.h"
#import "LynxLog.h"
#import "LynxServiceDevToolProtocol.h"
#import "LynxServiceEventReporterProtocol.h"
#import "LynxServiceExtensionProtocol.h"
#import "LynxServiceHttpProtocol.h"
#import "LynxServiceI18nProtocol.h"
#import "LynxServiceImageProtocol.h"
#import "LynxServiceLogProtocol.h"
#import "LynxServiceMonitorProtocol.h"
#import "LynxServiceProtocol.h"
#import "LynxServiceResourceProtocol.h"
#import "LynxServiceSecurityProtocol.h"
#import "LynxServiceSystemInvokeProtocol.h"
#import "LynxServiceTrailProtocol.h"
#import "LynxTraceEvent.h"

#import <objc/runtime.h>

static NSArray<NSString *> *lynx_services = nil;
static NSString *const LYNX_AUTO_REGISTER_SERVICE_PREFIX = @"__lynx_auto_register_serivce__";

@interface LynxServices ()

@property(atomic, strong) NSMutableDictionary<NSString *, Class> *protocolToClassMap;
@property(atomic, strong) NSMutableDictionary<NSString *, id> *protocolToInstanceMap;
@property(atomic, strong) NSRecursiveLock *recLock;
@property(atomic, strong) NSMutableSet<NSString *> *protocolClassCalledSet;

@end

@implementation LynxVerificationResult

@end

@implementation LynxServices

LYNX_LOAD_LAZY(static dispatch_once_t onceToken; dispatch_once(&onceToken, ^{
                 LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxServices registerServices")
                 NSMutableArray<NSString *> *autoRegisteredService = [NSMutableArray new];
                 unsigned int methodCount = 0;
                 Method *methods =
                     class_copyMethodList(object_getClass([self class]), &methodCount);
                 for (unsigned int i = 0; i < methodCount; i++) {
                   Method method = methods[i];
                   SEL selector = method_getName(method);
                   if ([NSStringFromSelector(selector)
                           hasPrefix:LYNX_AUTO_REGISTER_SERVICE_PREFIX]) {
                     IMP imp = method_getImplementation(method);
                     NSString *className = ((NSString * (*)(id, SEL)) imp)(self, selector);
                     if ([className isKindOfClass:[NSString class]] && className.length > 0) {
                       [autoRegisteredService addObject:className];
                       Class cls = NSClassFromString(className);
                       // check is subclass of LynxService?
                       NSAssert([cls conformsToProtocol:@protocol(LynxServiceProtocol)],
                                @"service class should conforms to LynxServiceProtocol");
                       [LynxServices registerService:cls];
                     }
                   }
                 }
                 free(methods);
                 lynx_services = [autoRegisteredService copy];
                 LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
               });)

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static LynxServices *services;
  dispatch_once(&onceToken, ^{
    services = [[LynxServices alloc] init];
  });
  return services;
}

+ (void)registerService:(Class)cls {
  if (cls == nil || ![cls conformsToProtocol:@protocol(LynxServiceProtocol)]) {
    NSAssert(NO, @"%@ should conforms to %@, %s", NSStringFromClass(cls),
             NSStringFromProtocol(@protocol(LynxServiceProtocol)), __func__);
    return;
  }
  Protocol *protocol = [self getProtocolByServiceType:[cls serviceType]];
  if (protocol != nil) {
    [[self sharedInstance] bindClass:cls toProtocol:protocol];
  } else {
    LLogInfo(@"Unknow lynx service type - %lu", [cls serviceType]);
  }
}

+ (Protocol *)getProtocolByServiceType:(NSUInteger)serviceType {
  switch (serviceType) {
    case kLynxServiceTypeMonitor:
      return @protocol(LynxServiceMonitorProtocol);
    case kLynxServiceHttp:
      return @protocol(LynxServiceHttpProtocol);
    case kLynxServiceTrail:
      return @protocol(LynxServiceTrailProtocol);
#ifdef OS_IOS
    case kLynxServiceImage:
      return @protocol(LynxServiceImageProtocol);
#endif
    case kLynxServiceEventReporter:
      return @protocol(LynxServiceEventReporterProtocol);
    case kLynxServiceSystemInvoke:
      return @protocol(LynxServiceSystemInvokeProtocol);
    case kLynxServiceModule:
      return @protocol(LynxServiceModuleProtocol);
    case kLynxServiceLog:
      return @protocol(LynxServiceLogProtocol);
    case kLynxServiceI18n:
      return @protocol(LynxServiceI18nProtocol);
    case kLynxServiceResource:
      return @protocol(LynxServiceResourceProtocol);
    case kLynxServiceDevTool:
      return @protocol(LynxServiceDevToolProtocol);
    case kLynxServiceExtension:
      return @protocol(LynxServiceExtensionProtocol);
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
    _protocolClassCalledSet = [[NSMutableSet alloc] init];
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
  if (![cls conformsToProtocol:@protocol(LynxServiceProtocol)]) {
    NSAssert(NO, @"%@ should conforms to LynxServiceProtocol, %s", NSStringFromClass(cls),
             __func__);
    return;
  }
  [self.recLock lock];
  // Get biz name from clz instance
  // key: protocolName__lynx_binder__${bizID}
  NSString *protocolName =
      [NSString stringWithFormat:@"%@__lynx_binder__%@", NSStringFromProtocol(protocol),
                                 [cls serviceBizID] ?: DEFAULT_LYNX_SERVICE];
  if ([self.protocolToClassMap objectForKey:protocolName] == nil) {
    [self.protocolToClassMap setObject:cls forKey:protocolName];
  } else {
    NSAssert(NO, @"%@ and %@ are duplicated bindings, %s", NSStringFromClass(cls),
             NSStringFromProtocol(protocol), __func__);
  }
  [self.recLock unlock];
}

- (id)getInstanceWithProtocol:(Protocol *)protocol bizID:(NSString *)bizID {
  if (lynx_services == nil || lynx_services.count == 0) {
    return nil;
  }
  [self.recLock lock];
  // key: protocolName__lynx_binder__${bizID}
  NSString *finalBizID = bizID ?: DEFAULT_LYNX_SERVICE;
  NSString *protocolName = [NSString
      stringWithFormat:@"%@__lynx_binder__%@", NSStringFromProtocol(protocol), finalBizID];
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
