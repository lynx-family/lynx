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

NSString *const LOCAL_URL_PREFIX = @"file://lynx?local://";
NSString *const HOMEPAGE_URL =
    @"file://lynx?local://homepage.lynx.bundle?fullscreen=true&orientation=portrait";

@interface AppDelegate ()

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
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

  [LynxDebugger setOpenCardCallback:^(NSString *url) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self openCard:url];
    });
  }];

  // Check for initial URL from environment variable.
  NSDictionary *env = [[NSProcessInfo processInfo] environment];
  NSString *initialUrl = env[@"lynx_initial_url"];
  if (initialUrl && initialUrl.length > 0) {
    [self openCard:initialUrl];
    return YES;
  }

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

- (BOOL)application:(UIApplication *)app
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
  NSString *scheme = [url scheme];

  // Handle lynx://open?url=https://... for runtime page switching.
  if ([scheme isEqualToString:@"lynx"] && [[url host] isEqualToString:@"open"]) {
    // Parse query string manually (NSURLComponents doesn't handle nested URLs well).
    NSString *queryString = [url query];
    NSString *targetUrl = nil;
    if (queryString) {
      NSString *prefix = @"url=";
      NSRange range = [queryString rangeOfString:prefix];
      if (range.location != NSNotFound) {
        targetUrl = [queryString substringFromIndex:range.location + range.length];
      }
    }
    if (targetUrl && targetUrl.length > 0) {
      [self openCard:targetUrl];
      return YES;
    }
  }

  return NO;
}

@end
