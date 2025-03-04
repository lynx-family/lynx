// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLifecycleDispatcher.h"
#import "LynxEnv.h"
#import "LynxEventReporter.h"
#import "LynxView+Internal.h"
#import "LynxViewClient.h"

@implementation LynxLifecycleDispatcher {
  NSHashTable<id<LynxViewBaseLifecycle>>* _innerLifecycleClients;
  NSNumber* _enableLifecycleTimeReport;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _innerLifecycleClients = [NSHashTable hashTableWithOptions:NSPointerFunctionsWeakMemory];
    _enableLifecycleTimeReport = nil;
    _instanceId = kUnknownInstanceId;
  }

  return self;
}

- (void)dummyMethod {
};

- (void)addLifecycleClient:(id<LynxViewBaseLifecycle>)lifecycleClient {
  @synchronized(self) {
    if (lifecycleClient && ![_innerLifecycleClients containsObject:lifecycleClient]) {
      [_innerLifecycleClients addObject:lifecycleClient];
    }
  }
}

- (void)removeLifecycleClient:(id<LynxViewBaseLifecycle>)lifecycleClient {
  @synchronized(self) {
    if (lifecycleClient && [_innerLifecycleClients containsObject:lifecycleClient]) {
      [_innerLifecycleClients removeObject:lifecycleClient];
    }
  }
}

- (NSArray<id<LynxViewBaseLifecycle>>*)lifecycleClients {
  NSMutableArray<id<LynxViewBaseLifecycle>>* clients = [[NSMutableArray alloc] init];
  @synchronized(self) {
    @autoreleasepool {
      for (id<LynxViewBaseLifecycle> client in _innerLifecycleClients) {
        [clients addObject:client];
      }
    }
  }

  return clients;
}

- (BOOL)getEnableLifecycleTimeReport {
  // Since boolFromExternalEnv owns a pthread_mutex_lock, we get LynxEnvEnableLifecycleTimeReport
  // only once.
  if (_enableLifecycleTimeReport == nil) {
    _enableLifecycleTimeReport =
        @([[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableLifecycleTimeReport
                                           defaultValue:NO]);
  }
  return [_enableLifecycleTimeReport boolValue];
}

- (void)lynxView:(LynxView*)view didRecieveError:(NSError*)error {
  // Client will retrieve error info through LynxError.userInfo.
  // We invoke 'userInfo' to pre-generate the json string cache
  // for LynxError to avoid race conditions in clients.
  if (!error || !error.userInfo) {
    return;
  }

  NSTimeInterval startTime = -1;
  if ([self getEnableLifecycleTimeReport]) {
    startTime = [NSDate timeIntervalSinceReferenceDate];
  }

  NSArray* allLifecycleClients = self.lifecycleClients;
  [allLifecycleClients
      enumerateObjectsUsingBlock:^(id _Nonnull client, NSUInteger idx, BOOL* _Nonnull stop) {
        if ([client respondsToSelector:@selector(lynxView:didRecieveError:)] ||
            [client isKindOfClass:LynxLifecycleDispatcher.class]) {
          [client lynxView:view didRecieveError:error];
        }
      }];

  if (startTime != -1) {
    [self handleClientMetricsWithStartTime:startTime blockName:@"didRecieveError"];
  }
}

- (void)handleClientMetricsWithStartTime:(NSTimeInterval)startTime blockName:(NSString*)name {
  NSTimeInterval endTime = [NSDate timeIntervalSinceReferenceDate];
  NSTimeInterval duration = endTime - startTime;

  NSString* threadName = @"";
  if ([[NSThread currentThread] isMainThread]) {
    threadName = @"main";
  } else {
    threadName = [NSThread currentThread].name;
  }
  NSDictionary* props = @{
    @"name" : name,
    @"duration" : @(duration * 1000),  // in milliseconds
    @"thread" : threadName
  };
  [LynxEventReporter onEvent:@"lynxsdk_lifecycle_time" instanceId:_instanceId props:props];
}

- (void)forwardInvocation:(NSInvocation*)invocation {
  SEL sel = invocation.selector;

  NSTimeInterval startTime = -1;
  if ([self getEnableLifecycleTimeReport]) {
    startTime = [NSDate timeIntervalSinceReferenceDate];
  }

  NSArray* allLifecycleClients = self.lifecycleClients;
  [allLifecycleClients enumerateObjectsUsingBlock:^(id _Nonnull client, NSUInteger idx,
                                                    BOOL* _Nonnull stop) {
    if ([client respondsToSelector:sel] || [client isKindOfClass:LynxLifecycleDispatcher.class]) {
      [invocation invokeWithTarget:client];
    }
  }];

  if (startTime != -1) {
    NSString* functionName = NSStringFromSelector(sel);
    [self handleClientMetricsWithStartTime:startTime blockName:functionName];
  }
}

// issue: #1510
// see https://developer.apple.com/documentation/objectivec/nsobject/1571955-forwardinvocation
// CAUTION:
// To respond to methods that your object does not itself recognize, you must override
// methodSignatureForSelector: If methodSignatureForSelector: is not overrided, an exception with
// message: unrecognized selector sent to instance will be thrown
- (NSMethodSignature*)methodSignatureForSelector:(SEL)sel {
  for (id sub in self.lifecycleClients) {
    if ([sub respondsToSelector:sel]) {
      NSMethodSignature* signature = [sub methodSignatureForSelector:sel];
      if (signature) {
        return signature;
      }
    }
  }
  return [LynxLifecycleDispatcher instanceMethodSignatureForSelector:@selector(dummyMethod)];
}

#if OS_OSX
- (void)lynxViewDidConstructJSRuntime:(LynxView*)view {
  for (id sub in self.lifecycleClients) {
    [sub lynxViewDidConstructJSRuntime:view];
  }
}
#endif

@end
