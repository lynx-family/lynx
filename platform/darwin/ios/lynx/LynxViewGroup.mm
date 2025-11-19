// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLog.h>
#import <Lynx/LynxLogicExecutor.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewGroup.h>
#import <pthread.h>
#include <atomic>

@implementation LynxViewGroup {
  LynxTemplateBundle *_templateBundle;
  NSError *_fetchError;
  std::atomic<int> _nextLynxViewId;
  NSMapTable<NSNumber *, LynxView *> *_viewMap;
  pthread_rwlock_t _viewMapLock;
  dispatch_group_t _fetchTask;
  std::atomic_bool _hasTimeout;

  NSMutableArray<LynxTemplateBundleResultBlock> *_callbacks;
  dispatch_queue_t _callbacksDispatchQueue;
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
  _fetchTask = dispatch_group_create();
  _callbacksDispatchQueue =
      dispatch_queue_create("lynx.template.fetch.callbacks", DISPATCH_QUEUE_SERIAL);
  _callbacks = [[NSMutableArray alloc] init];
  if (bundle == nil) {
    // no template bundle provided, start a fetch task
    dispatch_group_enter(_fetchTask);
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
  dispatch_sync(_callbacksDispatchQueue, ^{
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
    dispatch_group_leave(_fetchTask);
  });
  for (LynxTemplateBundleResultBlock cb in callbacksCopy) {
    cb(bundle, error);
  }
}

- (void)fetchTemplate:(LynxTemplateBundleResultBlock)callback {
  if (_templateBundle) {
    callback(_templateBundle, nil);
    return;
  }
  dispatch_sync(_callbacksDispatchQueue, ^{
    // double check
    if (_templateBundle) {
      callback(_templateBundle, nil);
      return;
    }
    [_callbacks addObject:[callback copy]];
  });
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
  if (dispatch_group_wait(_fetchTask, wait) != 0) {
    _hasTimeout = true;
  }
  return _templateBundle;
}

- (void)setTemplateBundle:(LynxTemplateBundle *_Nullable)templateBundle {
  _templateBundle = templateBundle;
}

- (void)setLogicExecutor:(id<LynxLogicExecutor>)logicExecutor {
  _logicExecutor = logicExecutor;
  [_logicExecutor setLynxViewGroup:self];
}

- (LynxTemplateBundle *_Nullable)getTemplateBundleNonBlocking {
  return _templateBundle;
}

@end
