// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "DevToolLogBoxProxy.h"
#import "DevToolLogBoxEnv.h"
#import "DevToolLogBoxHelper.h"
#import "DevToolLogBoxOwner.h"

@implementation DevToolLogBoxProxy {
  __weak id<DevToolLogBoxResProvider> _resProvider;
#if OS_IOS
  __weak UIViewController* _pageViewController;
#else
  __strong UIViewController* _pageViewController;
#endif
  BOOL _needFlush;
  NSString* _errorNamespace;
}

- (instancetype)initWithNamespace:(NSString*)errNamespace
                 resourceProvider:(id<DevToolLogBoxResProvider>)provider {
  NSLog(@"LogBoxProxy: initWithNamespace:%@ resourceProvider: %p", errNamespace, provider);
  self = [super init];
  if (self) {
    _resProvider = provider;
    _logMessages = [NSMutableDictionary dictionary];
    _needFlush = NO;
    _errorNamespace = errNamespace;
  }
  return self;
}

- (void)registerErrorParserWithBundle:(NSString*)bundleUrl file:(NSString*)filePath {
  [[DevToolLogBoxEnv sharedInstance]
      registerErrorParserLoader:^(DevToolFileLoadCallback callback) {
        [DevToolFileLoadUtils loadFileFromBundle:bundleUrl
                                        filePath:filePath
                                            type:@".js"
                                      completion:callback];
      }
                  withNamespace:_errorNamespace];
}

- (UIViewController*)pageViewController {
  if (_pageViewController) {
    return _pageViewController;
  }

#if OS_OSX
  _pageViewController = [[UIViewController alloc] init];
  return _pageViewController;
#endif

  __strong id<DevToolLogBoxResProvider> provider = _resProvider;
  if (!provider) {
    return nil;
  }

  UIView* view = [provider getView];
  if (!view) {
    return nil;
  }
  UIResponder* next = [view nextResponder];
  do {
    if ([next isKindOfClass:[UIViewController class]]
#if OS_IOS
        && ![next isKindOfClass:[UITabBarController class]]
#endif
    ) {
      _pageViewController = (UIViewController*)next;
      return _pageViewController;
    }
    next = [next nextResponder];
  } while (next != nil);
  return nil;
}

- (void)onMovedToWindow {
  NSLog(@"logbox: onMovedToWindow, self: %p", self);
  if (_needFlush) {
    [self flushLogMessages];
  }
}

- (void)flushLogMessages {
  [_logMessages
      enumerateKeysAndObjectsUsingBlock:^(NSString* _Nonnull level, NSMutableArray* _Nonnull msgArr,
                                          BOOL* _Nonnull stop) {
        for (NSString* message in msgArr) {
          [self flushLogMessage:message withLevel:level];
        }
      }];
  _needFlush = NO;
}

- (void)showLogMessage:(NSString*)error withLevel:(NSString*)level {
  if ([NSThread isMainThread]) {
    [self showLogMessageOnMainThread:error withLevel:level];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf showLogMessageOnMainThread:error withLevel:level];
    });
  }
}

- (void)showLogMessageOnMainThread:(NSString*)message withLevel:(NSString*)level {
  NSString* logLevel = [level isEqualToString:@"warn"] ? @"warn" : @"error";
  if ([_logMessages objectForKey:logLevel] == nil) {
    [_logMessages setObject:[NSMutableArray new] forKey:logLevel];
  }
  [[_logMessages objectForKey:logLevel] addObject:message];
  [self flushLogMessage:message withLevel:logLevel];
}

- (void)flushLogMessage:(NSString*)message withLevel:(NSString*)level {
  [self prepareOwner];
  [[DevToolLogBoxOwner getInstance] onNewLog:message withLevel:level withProxy:self];
}

- (void)prepareOwner {
  UIViewController* controller = [self pageViewController];
  if (!controller) {
    NSLog(@"logbox: UIViewController is nil! self: %p, ResourceProvider: %p", self, _resProvider);
    _needFlush = YES;
    return;
  }
  [[DevToolLogBoxOwner getInstance] insertLogBoxProxy:self withController:controller];
}

- (void)onResourceProviderReady {
  if ([NSThread isMainThread]) {
    [[DevToolLogBoxOwner getInstance] updateEntryUrlForLogSrc:[self entryUrlForLogSrc]
                                                    withProxy:self];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [[DevToolLogBoxOwner getInstance] updateEntryUrlForLogSrc:[strongSelf entryUrlForLogSrc]
                                                      withProxy:strongSelf];
    });
  }
}

- (void)reset {
  [[DevToolLogBoxOwner getInstance] onProxyReset:self];
  [self clearMsg];
}

- (void)clearMsg {
  [_logMessages removeAllObjects];
}

- (NSMutableArray*)logMessagesWithLevel:(NSString*)level {
  return [_logMessages objectForKey:level];
}

- (void)removeLogMessagesWithLevel:(NSString*)level {
  [_logMessages removeObjectForKey:level];
}

- (NSDictionary*)logSources {
  __strong id<DevToolLogBoxResProvider> provider = _resProvider;
  return [provider logSources];
}

- (NSString*)logSourceWithFileName:(NSString*)fileName {
  __strong id<DevToolLogBoxResProvider> provider = _resProvider;
  return [provider logSourceWithFileName:fileName];
}

- (NSString*)entryUrlForLogSrc {
  __strong id<DevToolLogBoxResProvider> provider = _resProvider;
  return [provider entryUrlForLogSrc];
}

- (NSString*)getErrorNamespace {
  return _errorNamespace;
}

- (void)destroy {
#if OS_IOS
  if (_needFlush) {
    NSLog(@"logbox: destroy, flushLogMessages: %p", self);
    UIWindow* window = [[[UIApplication sharedApplication] delegate] window];
    UIViewController* current = window.rootViewController;

    while (current) {
      if ([current isKindOfClass:[UITabBarController class]]) {
        UITabBarController* tabBarController = (UITabBarController*)current;
        current = tabBarController.selectedViewController;
      } else if ([current isKindOfClass:[UINavigationController class]]) {
        UINavigationController* navigationController = (UINavigationController*)current;
        current = navigationController.topViewController;
      } else if ([current presentedViewController]) {
        current = [current presentedViewController];
      } else {
        break;
      }
    }
    _pageViewController = current;

    [self flushLogMessages];
  }
#endif
}

@end
