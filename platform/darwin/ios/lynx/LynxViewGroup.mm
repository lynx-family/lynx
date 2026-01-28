// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxConfig+internal.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxLogicExecutor.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewGroup.h>
#import <pthread.h>
#include <atomic>

#include "core/runtime/js/bindings/modules/ios/module_factory_darwin.h"

@implementation LynxViewGroup {
  LynxTemplateBundle *_templateBundle;
  NSError *_fetchError;
  std::atomic<int> _nextLynxViewId;
  NSMapTable<NSNumber *, LynxView *> *_viewMap;
  NSMutableArray<LynxTemplateBundleResultBlock> *_callbacks;
  pthread_mutex_t _callbacksLock;
  pthread_rwlock_t _viewMapLock;
  dispatch_group_t _fetch_task;
  std::atomic_bool _hasTimeout;
}

- (instancetype)init:(nonnull NSString *)url
      templateBundle:(nullable LynxTemplateBundle *)bundle
     templateFetcher:(id<LynxTemplateResourceFetcher>)templateFetcher {
  if (!(self = [super init])) {
    return nil;
  }
  self.url = url;
  self.templateResourceFetcher = templateFetcher;
  _templateBundle = bundle;
  _nextLynxViewId = 1;
  _viewMap = [NSMapTable strongToWeakObjectsMapTable];
  pthread_rwlock_init(&_viewMapLock, nil);
  _fetch_task = dispatch_group_create();
  pthread_mutex_init(&_callbacksLock, NULL);
  _callbacks = [[NSMutableArray alloc] init];
  if (bundle == nil) {
    // no template bundle provided, start a fetch task
    dispatch_group_enter(_fetch_task);
    [self fetchTemplateInternal];
  }
  return self;
}

- (instancetype)initWithUrl:(nonnull NSString *)url
            templateFetcher:(id<LynxTemplateResourceFetcher>)templateFetcher {
  return [[LynxViewGroup alloc] init:url templateBundle:nil templateFetcher:templateFetcher];
}

- (instancetype)initWithUrl:(nonnull NSString *)url
             templateBundle:(nonnull LynxTemplateBundle *)bundle {
  return [[LynxViewGroup alloc] init:url templateBundle:bundle templateFetcher:nil];
}

- (bool)isTemplateBundleReady {
  return _templateBundle != nil;
}

- (void)destroyForInstance:(NSString *)instanceId {
  auto moduleFactoryPtr = [_config getSharedModuleFactoryPtr];
  if (moduleFactoryPtr) {
    moduleFactoryPtr->DeleteLynxContextForInstance(instanceId);
  }
}

- (int)generateNextLynxViewID {
  return _nextLynxViewId++;
}
- (nullable LynxView *)getLynxViewById:(int)viewId {
  pthread_rwlock_rdlock(&_viewMapLock);
  LynxView *view = [_viewMap objectForKey:@(viewId)];
  pthread_rwlock_unlock(&_viewMapLock);
  return view;
}

- (void)addLynxView:(int)lynxViewId view:(LynxView *)view {
  pthread_rwlock_wrlock(&_viewMapLock);
  [_viewMap setObject:view forKey:@(lynxViewId)];
  pthread_rwlock_unlock(&_viewMapLock);
}

- (void)removeLynxView:(int)lynxViewId {
  pthread_rwlock_wrlock(&_viewMapLock);
  [_viewMap removeObjectForKey:@(lynxViewId)];
  pthread_rwlock_unlock(&_viewMapLock);
}

- (void)fetchTemplateInternal {
  if (_templateBundle != nil) {
    NSAssert(false, @"template bundle has been assigned");
    return;
  }
  if (self.templateResourceFetcher == nil) {
    NSAssert(false, @"no resource fetcher found for template fetching");
    return;
  }

  LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:self.url
                                                                     type:LynxResourceTypeTemplate];
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    [self.templateResourceFetcher
        fetchTemplate:request
           onComplete:^(LynxTemplateResource *_Nullable data, NSError *_Nullable error) {
             __strong typeof(weakSelf) strongSelf = weakSelf;
             if (!strongSelf) {
               return;
             }
             if (error) {
               [strongSelf setFetchResult:nil error:error];
               return;
             }
             if (data.bundle) {
               [strongSelf setFetchResult:data.bundle error:nil];
             } else if (data.data) {
               [strongSelf setFetchResult:[[LynxTemplateBundle alloc] initWithTemplate:data.data]
                                    error:nil];
             } else {
               LLogError(@"failed to fetch template: empty data, url=%@", strongSelf.url);
               [strongSelf
                   setFetchResult:nil
                            error:[NSError errorWithDomain:@"unknown error"
                                                      code:1
                                                  userInfo:@{
                                                    NSLocalizedFailureReasonErrorKey :
                                                        @"failed to fetch template: empty data"
                                                  }]];
             }
           }];
  });
}

- (void)setFetchResult:(nullable LynxTemplateBundle *)bundle error:(nullable NSError *)error {
  __block NSArray<LynxTemplateBundleResultBlock> *callbacksCopy = nil;
  @try {
    pthread_mutex_lock(&_callbacksLock);
    if (_templateBundle != nil || _fetchError != nil) {
      LLogError(@"internal error: fetch result should be set once");
      return;
    }
    if (error) {
      LLogError(@"failed to fetch template: %@, url=%@", error, _url);
      _fetchError = error;
    } else {
      [self setTemplateBundle:bundle];
    }
    callbacksCopy = [_callbacks copy];
    [_callbacks removeAllObjects];
    dispatch_group_leave(_fetch_task);
  } @finally {
    pthread_mutex_unlock(&_callbacksLock);
  }
  for (LynxTemplateBundleResultBlock cb in callbacksCopy) {
    cb(bundle, error);
  }
}

- (void)fetchTemplate:(LynxTemplateBundleResultBlock)callback {
  if (_templateBundle != nil) {
    callback(_templateBundle, nil);
    return;
  }
  if (_fetchError != nil) {
    callback(nil, _fetchError);
    return;
  }
  @try {
    pthread_mutex_lock(&_callbacksLock);
    // double check
    if (_templateBundle) {
      callback(_templateBundle, nil);
      return;
    }
    if (_fetchError != nil) {
      callback(nil, _fetchError);
      return;
    }
    [_callbacks addObject:[callback copy]];
  } @finally {
    pthread_mutex_unlock(&_callbacksLock);
  }
}

- (nullable LynxTemplateBundle *)templateBundle {
  if (_templateBundle) {
    return _templateBundle;
  }
  if (_hasTimeout) {
    // If waiting timeout has occurred previously, return early to avoid redundant waits
    return nil;
  }
  dispatch_time_t wait = dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC);
  if (dispatch_group_wait(_fetch_task, wait) != 0) {
    _hasTimeout = true;
  }
  return _templateBundle;
}

- (void)setTemplateBundle:(LynxTemplateBundle *_Nullable)templateBundle {
  _templateBundle = templateBundle;
}

- (void)setLogicExecutor:(id<LynxLogicExecutor>)logicExecutor {
  _logicExecutor = logicExecutor;
}

- (LynxTemplateBundle *_Nullable)getTemplateBundleNonBlocking {
  return _templateBundle;
}

- (void)setConfig:(LynxConfig *)config {
  [super setConfig:config];
  [LynxService(LynxServiceModuleProtocol) initLynxViewGroup:self];
}

- (void)registerModule:(nonnull Class<LynxModule>)module {
  [_config registerModule:module];
}

- (void)registerModule:(nonnull Class<LynxModule>)module param:(nullable id)param {
  [_config registerModule:module param:param];
}

@end
