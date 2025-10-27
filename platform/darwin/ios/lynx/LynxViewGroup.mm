// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLog.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewGroup.h>
#import <pthread.h>

@implementation LynxViewGroup {
  LynxTemplateBundle *_templateBundle;
  int _nextLynxViewId;
  NSMapTable<NSNumber *, LynxView *> *_viewMap;
  pthread_rwlock_t _viewMapLock;
  dispatch_group_t _fetch_task;
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
  if (bundle == nil) {
    // no template bundle provided, start a fetch task
    dispatch_group_enter(_fetch_task);
    [self fetchTemplate];
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

- (nullable LynxView *)getLynxViewById:(NSInteger)viewId {
  pthread_rwlock_rdlock(&_viewMapLock);
  LynxView *view = [_viewMap objectForKey:@(viewId)];
  pthread_rwlock_unlock(&_viewMapLock);
  return view;
}

- (void)addLynxView:(nonnull LynxView *)view {
  pthread_rwlock_wrlock(&_viewMapLock);
  view.lynxViewId = _nextLynxViewId++;
  [_viewMap setObject:view forKey:@(view.lynxViewId)];
  pthread_rwlock_unlock(&_viewMapLock);
}

- (void)fetchTemplate {
  if (_templateBundle != nil) {
    NSAssert(false, @"template bundle has been assigned");
    return;
  }
  if (self.templateResourceFetcher == nil) {
    NSAssert(false, @"no resource fetcher found for template fetching");
    return;
  }

  LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:self.url];
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    [self.templateResourceFetcher
        fetchTemplate:request
           onComplete:^(LynxTemplateResource *_Nullable data, NSError *_Nullable error) {
             __strong typeof(weakSelf) strongSelf = weakSelf;
             @try {
               if (!strongSelf) {
                 return;
               }
               if (error) {
                 LLogError(@"failed to fetch template: %@, url=%@", error, strongSelf.url);
                 return;
               }
               if (data.bundle) {
                 strongSelf->_templateBundle = data.bundle;
               } else if (data.data) {
                 strongSelf->_templateBundle =
                     [[LynxTemplateBundle alloc] initWithTemplate:data.data];
               } else {
                 LLogError(@"failed to fetch template: empty data, url=%@", strongSelf.url);
               }
             } @finally {
               dispatch_group_leave(strongSelf->_fetch_task);
             }
           }];
  });
}

- (nullable LynxTemplateBundle *)templateBundle {
  if (_templateBundle) {
    return _templateBundle;
  }
  dispatch_time_t wait = dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC);
  dispatch_group_wait(_fetch_task, wait);
  return self->_templateBundle;
}

@end
