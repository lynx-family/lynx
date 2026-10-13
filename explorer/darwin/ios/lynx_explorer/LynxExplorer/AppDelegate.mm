// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "AppDelegate.h"
#if __has_include("Sparkling-umbrella.h")
#import "Sparkling-umbrella.h"
#import "SparklingMacro-umbrella.h"
#define HAS_SPARKLING 1
#endif
#import "LynxDebugger.h"
#import "LynxExplorerSwiftInterop.h"
#import "LynxInitProcessor.h"

#if __has_include("addon_use.h")
#include "addon_use.h"
#endif

// PrimJS and LynxWeakNodeAPI expose a C bridge for sharing the weak Node-API
// raw host pointer at app startup.
extern "C" {
typedef const void *(*PrimJSWeakNodeApiRawPtrHostProvider)(void);
void PrimJSInstallWeakNodeApiRawPtrHostProvider(PrimJSWeakNodeApiRawPtrHostProvider provider);
void SetupWeakNodeApiEnv(void);
const void *PrimJSGetWeakNodeApiRawPtrHost(void);
}

static const void *PrimJSProvideWeakNodeApiRawPtrHost(void) {
  return PrimJSGetWeakNodeApiRawPtrHost();
}

static void InstallPrimJSWeakNodeApiBridge(void) {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    // Register PrimJS as the raw host provider before Lynx creates any runtime.
    PrimJSInstallWeakNodeApiRawPtrHostProvider(PrimJSProvideWeakNodeApiRawPtrHost);
    // Initialize the weak Node-API side once so later runtimes can reuse it.
    SetupWeakNodeApiEnv();
  });
}

NSString *const HOMEPAGE_URL = @"file://lynx?local://homepage.lynx.bundle?fullscreen=true";

@interface AppDelegate ()

@property(nonatomic, strong) LXRouteCoordinator *routeCoordinator;

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions {
  InstallPrimJSWeakNodeApiBridge();
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

  self.routeCoordinator =
      [LXExplorerRouteComposition makeBridgeWithNavigationController:self.navigationController];
  [LXRouteCoordinator installBridge:self.routeCoordinator];

  __weak AppDelegate *weakSelf = self;
  [LynxDebugger addOpenCardCallback:^(NSString *url) {
    __strong AppDelegate *strongSelf = weakSelf;
    [strongSelf.routeCoordinator openDebugURL:url
                                     callback:^(__unused id payload){
                                     }];
  }];

  NSDictionary<NSString *, NSString *> *environment = NSProcessInfo.processInfo.environment;
  BOOL isRunningTests = environment[@"XCTestConfigurationFilePath"] != nil ||
                        environment[@"XCInjectBundleInto"] != nil;
  [self.routeCoordinator openStartupWithEnvironment:environment
                                      launchOptions:launchOptions ?: @{}
                                        homepageURL:HOMEPAGE_URL
                                     isRunningTests:isRunningTests
                                           callback:^(__unused id payload){
                                           }];
  return YES;
}

- (BOOL)application:(UIApplication *)app
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
  if (url.scheme.length == 0) {
    return NO;
  }
  [self.routeCoordinator openExternalURL:url
                                callback:^(__unused id payload){
                                }];
  return YES;
}

- (BOOL)application:(UIApplication *)application
    continueUserActivity:(NSUserActivity *)userActivity
      restorationHandler:
          (void (^)(NSArray<id<UIUserActivityRestoring>> *_Nullable))restorationHandler {
  NSURL *webpageURL = userActivity.webpageURL;
  if (webpageURL == nil) {
    return NO;
  }
  [self.routeCoordinator openUniversalLinkURL:webpageURL
                                     callback:^(__unused id payload){
                                     }];
  return YES;
}

@end
