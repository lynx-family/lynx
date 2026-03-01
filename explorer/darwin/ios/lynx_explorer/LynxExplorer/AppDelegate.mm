// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "AppDelegate.h"
#if __has_include("Sparkling-umbrella.h")
#import "Sparkling-umbrella.h"
#define HAS_SPARKLING 1
#endif
#import "LynxDebugger.h"
#import "LynxExplorer-Swift.h"
#import "LynxInitProcessor.h"
#import "TasmDispatcher.h"

NSString *const LOCAL_URL_PREFIX = @"file://lynx?local://";
NSString *const HOMEPAGE_URL =
    @"file://lynx?local://homepage.lynx.bundle?fullscreen=true&orientation=portrait";

@interface AppDelegate ()

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  [[LynxInitProcessor sharedInstance] setupEnvironment];

#if HAS_SPARKLING
  // Register DI services before boot tasks
  [SPKServiceRegistrar registerAll];
  SPKExecuteAllPrepareBootTask();
#endif

  self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  self.navigationController = [[UINavigationController alloc] init];
  [self navigationController].navigationBar.hidden = YES;
  [self.navigationController setNavigationBarHidden:YES];
  self.navigationController.navigationBar.translucent = YES;
  self.window.rootViewController = self.navigationController;
  self.window.backgroundColor = [UIColor whiteColor];
  [self.window makeKeyAndVisible];

  [LynxDebugger setOpenCardCallback:^(NSString *url) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self openCard:url];
    });
  }];

  [[TasmDispatcher sharedInstance] openTargetUrl:HOMEPAGE_URL];
  return YES;
}

- (void)openCard:(NSString *)url {
  if ([url hasPrefix:LOCAL_URL_PREFIX]) {
    [[TasmDispatcher sharedInstance] openTargetUrlSingleTop:url];
  } else {
    [self.navigationController popToRootViewControllerAnimated:YES];
    [[TasmDispatcher sharedInstance] openTargetUrl:url];
  }
}

@end
