// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLogBoxOwner.h"
#import "LynxLogBoxManager.h"

@interface LynxLogBoxOwner ()

- (void)showNotificationWithController:(UIViewController *)controller;
- (void)hideNotificationWithController:(UIViewController *)controller;

@end

#pragma mark - LynxViewControllerLifeListener
@interface LynxViewControllerLifeListener : UIViewController
@end

@implementation LynxViewControllerLifeListener

#if OS_IOS
- (void)viewWillAppear:(BOOL)animated {
  [[LynxLogBoxOwner getInstance] showNotificationWithController:[self parentViewController]];
}

- (void)viewWillDisappear:(BOOL)animated {
  [[LynxLogBoxOwner getInstance] hideNotificationWithController:[self parentViewController]];
}

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];
  // The frame may be changed by the host, so we reset the frame here every time.
  self.view.frame = CGRectMake(0, 0, 0, 0);
}

#else
- (void)viewWillAppear {
  [[LynxLogBoxOwner getInstance] showNotificationWithController:[self parentViewController]];
}

- (void)viewWillDisappear {
  [[LynxLogBoxOwner getInstance] hideNotificationWithController:[self parentViewController]];
}
#endif

@end

#pragma mark - LynxLogBoxOwner
@implementation LynxLogBoxOwner {
  NSMapTable *_controllerToManager;
  NSMapTable *_proxyToController;
}

+ (LynxLogBoxOwner *)getInstance {
  static LynxLogBoxOwner *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[self alloc] init];
  });
  return _instance;
}

- (nonnull instancetype)init {
  self = [super init];
  if (self) {
    _controllerToManager = [NSMapTable weakToStrongObjectsMapTable];
    _proxyToController = [NSMapTable weakToWeakObjectsMapTable];
  }
  return self;
}

- (void)insertLogBoxProxy:(LynxLogBoxProxy *)proxy withController:(UIViewController *)controller {
  if ([NSThread isMainThread]) {
    [self insertLogBoxProxyOnMainThread:proxy withController:controller];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf insertLogBoxProxyOnMainThread:proxy withController:controller];
    });
  }
}

- (void)insertLogBoxProxyOnMainThread:(LynxLogBoxProxy *)proxy
                       withController:(UIViewController *)controller {
  [_proxyToController setObject:controller forKey:proxy];
  if ([_controllerToManager objectForKey:controller] == nil) {
    [_controllerToManager setObject:[[LynxLogBoxManager alloc] initWithViewController:controller]
                             forKey:controller];

    LynxViewControllerLifeListener *listener = [[LynxViewControllerLifeListener alloc] init];
    [controller addChildViewController:listener];
#if OS_OSX
    listener.view = [[NSView alloc] init];
#endif
    [controller.view addSubview:listener.view];
#if OS_IOS
    [listener didMoveToParentViewController:controller];
#endif
  }
}

- (void)onNewLog:(NSString *)message
       withLevel:(LynxLogBoxLevel)level
       withProxy:(LynxLogBoxProxy *)proxy {
  LynxLogBoxManager *manager = [self getLogBoxManagerWithProxy:proxy];
  [manager onNewLog:message withLevel:level withProxy:proxy];
}

- (void)onNewConsole:(NSDictionary *)message withProxy:(LynxLogBoxProxy *)proxy {
  LynxLogBoxManager *manager = [self getLogBoxManagerWithProxy:proxy];
  [manager onNewConsole:message withProxy:proxy];
}

- (void)showConsoleMsgsWithProxy:(LynxLogBoxProxy *)proxy {
  LynxLogBoxManager *manager = [self getLogBoxManagerWithProxy:proxy];
  [manager showConsoleMsgsWithProxy:proxy];
}

- (void)updateTemplateUrl:(NSString *)url withProxy:(LynxLogBoxProxy *)proxy {
  LynxLogBoxManager *manager = [self getLogBoxManagerWithProxy:proxy];
  [manager updateTemplateUrl:url withProxy:proxy];
}

- (void)reloadLynxViewWithProxy:(LynxLogBoxProxy *)proxy {
  LynxLogBoxManager *manager = [self getLogBoxManagerWithProxy:proxy];
  [manager reloadWithProxy:proxy];
}

- (void)showNotificationWithController:(UIViewController *)controller {
  LynxLogBoxManager *manager = [_controllerToManager objectForKey:controller];
  [manager showNotification];
}

- (void)hideNotificationWithController:(UIViewController *)controller {
  LynxLogBoxManager *manager = [_controllerToManager objectForKey:controller];
  [manager hideNotification];
}

- (LynxLogBoxManager *)getLogBoxManagerWithProxy:(LynxLogBoxProxy *)proxy {
  UIViewController *controller = [_proxyToController objectForKey:proxy];
  return [_controllerToManager objectForKey:controller];
}

@end
