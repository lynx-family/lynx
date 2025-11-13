// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// #import <Lynx/LynxLog.h>
#import <LynxServiceAPI/LynxServiceProtocol.h>
#import <LynxServiceAPI/ServiceAPI.h>
#import <LynxServiceAPI/ServiceLazyLoad.h>

#import <objc/runtime.h>

@interface LynxServices ()

@property(atomic, strong) NSMutableDictionary<NSString *, Class> *protocolToClassMap;
@property(atomic, strong) NSMutableDictionary<NSString *, id> *protocolToInstanceMap;
@property(atomic, strong) NSRecursiveLock *recLock;

@end

@implementation LynxServices

SERVICE_LOAD_LAZY(static dispatch_once_t onceToken; dispatch_once(&onceToken, ^{
                    unsigned int methodCount = 0;
                    Method *methods =
                        class_copyMethodList(object_getClass([self class]), &methodCount);
                    NSString *prefix = @STRINGIFY_(LYNX_AUTO_REGISTER_SERVICE_PREFIX);
                    for (unsigned int i = 0; i < methodCount; i++) {
                      Method method = methods[i];
                      SEL selector = method_getName(method);
                      if ([NSStringFromSelector(selector) hasPrefix:prefix]) {
                        IMP imp = method_getImplementation(method);
                        LynxServiceEntry *entry =
                            ((LynxServiceEntry * (*)(id, SEL)) imp)(self, selector);
                        [LynxServices registerServiceWithProtocol:entry->classObj
                                                         protocol:entry->protocolObj];
                      }
                    }
                    free(methods);
                  });)

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static LynxServices *services;
  dispatch_once(&onceToken, ^{
    services = [[LynxServices alloc] init];
  });
  return services;
}

+ (void)registerServiceWithProtocol:(Class)cls protocol:(Protocol *)protocol {
  [[self sharedInstance] bindClass:cls toProtocol:protocol];
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

+ (id)getInstanceWithProtocol:(Protocol *)protocol {
  if (!protocol || protocol == nil) {
    return nil;
  }
  return [[self sharedInstance] getInstanceWithProtocol:protocol];
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
  NSString *protocolName = NSStringFromProtocol(protocol);
  if ([self.protocolToClassMap objectForKey:protocolName] == nil) {
    [self.protocolToClassMap setObject:cls forKey:protocolName];
  } else {
    NSAssert(NO, @"%@ and %@ are duplicated bindings, %s", NSStringFromClass(cls), protocolName,
             __func__);
  }
  [self.recLock unlock];
}

- (id)getInstanceWithProtocol:(Protocol *)protocol {
  [self.recLock lock];
  if ([self.protocolToClassMap count] == 0) {
    [self.recLock unlock];
    return nil;
  }
  NSString *protocolName = NSStringFromProtocol(protocol);
  id object = [self.protocolToInstanceMap objectForKey:protocolName];
  if (object == nil) {
    // may not create instance yet
    Class cls = [self.protocolToClassMap objectForKey:protocolName];
    if (cls != nil) {
      object = [self getInstanceWithClass:cls];
      if (object != nil) {
        [self.protocolToInstanceMap setObject:object forKey:protocolName];
      } else {
        NSAssert(NO, @"failed to create instance for class %@", NSStringFromClass(cls));
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
