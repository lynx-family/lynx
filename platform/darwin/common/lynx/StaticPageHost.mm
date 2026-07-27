// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxBase/LynxLog.h>

#import "StaticPageHost+Internal.h"

@interface StaticPageHost ()
@property(nonatomic) int32_t instanceId;
@property(nonatomic) BOOL registered;
@property(nonatomic) uint64_t generation;
@property(nonatomic, strong) NSDictionary<NSString*, id>* currentData;
@property(nonatomic, strong, nullable) NSDictionary<NSString*, id>* currentGlobalProps;
@property(nonatomic, strong, nullable) id<StaticPageInstance> pageInstance;
@property(nonatomic, copy) StaticPageTaskDispatcher taskDispatcher;
+ (BOOL)renderPageForInstance:(int32_t)instanceId;
@end

extern "C" bool LynxStaticPageHostRenderPage(int32_t instanceId) {
  return [StaticPageHost renderPageForInstance:instanceId];
}

@implementation StaticPageHost

+ (NSMapTable<NSNumber*, StaticPageHost*>*)hosts {
  static NSMapTable<NSNumber*, StaticPageHost*>* hosts;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    hosts = [NSMapTable strongToWeakObjectsMapTable];
  });
  return hosts;
}

+ (nullable StaticPageHost*)find:(int32_t)instanceId {
  @synchronized(self) {
    return [[self hosts] objectForKey:@(instanceId)];
  }
}

+ (BOOL)attach:(int32_t)instanceId instance:(id<StaticPageInstance>)instance {
  StaticPageHost* host = [self find:instanceId];
  if (!host) {
    return NO;
  }
  @synchronized(host) {
    if (!host.registered || host.pageInstance) {
      return NO;
    }
    host.pageInstance = instance;
    return YES;
  }
}

+ (BOOL)renderPageForInstance:(int32_t)instanceId {
  StaticPageHost* host = [self find:instanceId];
  return host && [host renderPage];
}

- (instancetype)initWithTaskDispatcher:(StaticPageTaskDispatcher)taskDispatcher {
  self = [super init];
  if (self) {
    _instanceId = -1;
    _currentData = @{};
    _taskDispatcher = [taskDispatcher copy];
  }
  return self;
}

- (nullable NSDictionary<NSString*, id>*)registerInstanceId:(int32_t)instanceId
                                                       data:(NSDictionary<NSString*, id>*)data
                                                globalProps:
                                                    (NSDictionary<NSString*, id>*)globalProps {
  @synchronized([self class]) {
    @synchronized(self) {
      [self unregister];
      _generation++;
      if (data) {
        if (_currentData.count == 0) {
          _currentData = data;
        } else {
          NSMutableDictionary<NSString*, id>* merged = [data mutableCopy];
          [merged addEntriesFromDictionary:_currentData];
          _currentData = [merged copy];
        }
      }
      if (globalProps) {
        if (!_currentGlobalProps) {
          _currentGlobalProps = globalProps;
        } else {
          NSMutableDictionary<NSString*, id>* merged = [globalProps mutableCopy];
          [merged addEntriesFromDictionary:_currentGlobalProps];
          _currentGlobalProps = [merged copy];
        }
      }
      _instanceId = instanceId;
      _pageInstance = nil;
      _registered = instanceId >= 0;
      if (_registered) {
        [[[self class] hosts] setObject:self forKey:@(instanceId)];
      }
      return _currentGlobalProps;
    }
  }
}

- (BOOL)isRegistered {
  @synchronized(self) {
    return _registered;
  }
}

- (void)updateMetaData:(NSDictionary<NSString*, id>*)data
           globalProps:(NSDictionary<NSString*, id>*)globalProps {
  id<StaticPageInstance> instance;
  uint64_t generation;
  NSDictionary<NSString*, id>* mergedData = nil;
  @synchronized(self) {
    if (data) {
      if (_currentData.count == 0) {
        _currentData = data;
      } else {
        NSMutableDictionary<NSString*, id>* merged = [_currentData mutableCopy];
        [merged addEntriesFromDictionary:data];
        _currentData = [merged copy];
      }
      mergedData = _currentData;
    }
    if (globalProps) {
      _currentGlobalProps = globalProps;
    }
    instance = _registered ? _pageInstance : nil;
    generation = _generation;
  }
  if (instance) {
    [self dispatchOperation:@"updateMetaData"
                       task:^{
                         if ([self isCurrentPageInstance:instance generation:generation]) {
                           [instance updateMetaData:mergedData globalProps:globalProps];
                         }
                       }];
  }
}

- (void)clear {
  id<StaticPageInstance> instance;
  @synchronized([self class]) {
    @synchronized(self) {
      [self unregister];
      _generation++;
      _instanceId = -1;
      _registered = NO;
      instance = _pageInstance;
      _pageInstance = nil;
      _currentData = @{};
      _currentGlobalProps = nil;
    }
  }
  if (instance) {
    [self dispatchOperation:@"destroy"
                       task:^{
                         [instance destroy];
                       }];
  }
}

- (BOOL)isCurrentPageInstance:(id<StaticPageInstance>)instance generation:(uint64_t)generation {
  @synchronized(self) {
    return _registered && _pageInstance == instance && _generation == generation;
  }
}

- (BOOL)renderPage {
  id<StaticPageInstance> instance;
  NSDictionary<NSString*, id>* data;
  NSDictionary<NSString*, id>* globalProps;
  @synchronized(self) {
    if (!_registered || !_pageInstance) {
      return NO;
    }
    instance = _pageInstance;
    data = _currentData;
    globalProps = _currentGlobalProps;
  }
  @try {
    [instance renderPage:data globalProps:globalProps];
    return YES;
  } @catch (NSException* exception) {
    [self logFailure:@"renderPage" exception:exception];
    return NO;
  }
}

- (void)dispatchOperation:(NSString*)operation task:(dispatch_block_t)task {
  @try {
    _taskDispatcher(^{
      @try {
        task();
      } @catch (NSException* exception) {
        [self logFailure:operation exception:exception];
      }
    });
  } @catch (NSException* exception) {
    [self logFailure:[operation stringByAppendingString:@" dispatch"] exception:exception];
  }
}

- (void)unregister {
  if (_instanceId >= 0 && [[[self class] hosts] objectForKey:@(_instanceId)] == self) {
    [[[self class] hosts] removeObjectForKey:@(_instanceId)];
  }
}

- (void)logFailure:(NSString*)operation exception:(NSException*)exception {
  _LogE(@"Static page direct %@ failed: %@: %@", operation, exception.name, exception.reason);
}

@end
