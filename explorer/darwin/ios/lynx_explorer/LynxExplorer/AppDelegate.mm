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
NSString *const HOMEPAGE_URL = @"file://lynx?local://homepage.lynx.bundle?fullscreen=true";

@interface AppDelegate ()

@end

@implementation AppDelegate

+ (BOOL)isDarkMode {
  if (@available(iOS 13.0, *)) {
    return UITraitCollection.currentTraitCollection.userInterfaceStyle == UIUserInterfaceStyleDark;
  }
  return NO;
}

/// Appends theme-aware query params (bar_color, title_color, back_button_style)
/// to a legacy URL if they are not already present.
+ (NSString *)appendThemeParams:(NSString *)url {
  BOOL isDark = [self isDarkMode];
  NSMutableString *result = [url mutableCopy];
  // In legacy URLs like "file://lynx?local://X.bundle?params", the first ?
  // is part of the scheme, not a query separator. Check for params after the
  // bundle path by looking past the LOCAL_URL_PREFIX.
  BOOL hasParams;
  if ([url hasPrefix:LOCAL_URL_PREFIX]) {
    hasParams = [[url substringFromIndex:LOCAL_URL_PREFIX.length] containsString:@"?"];
  } else {
    hasParams = [url containsString:@"?"];
  }
  NSString *sep = hasParams ? @"&" : @"?";
  if (![url containsString:@"bar_color="]) {
    [result appendFormat:@"%@bar_color=%@", sep, isDark ? @"181D25" : @"F0F2F5"];
    sep = @"&";
  }
  if (![url containsString:@"title_color="]) {
    [result appendFormat:@"%@title_color=%@", sep, isDark ? @"FFFFFF" : @"000000"];
    sep = @"&";
  }
  if (![url containsString:@"back_button_style="]) {
    [result appendFormat:@"%@back_button_style=%@", sep, isDark ? @"dark" : @"light"];
  }
  return result;
}

/// Converts a lynx://lynxview_page?bundle=X&k=v URL to file://lynx?local://X?k=v.
/// Returns nil if the URL is not a valid lynxview_page deeplink.
/// Additional query params (e.g., title, bar_color) are forwarded as-is.
+ (NSString *)legacyUrlFromDeeplink:(NSURL *)url {
  if (![[url scheme] isEqualToString:@"lynx"] || ![[url host] isEqualToString:@"lynxview_page"]) {
    return nil;
  }
  NSURLComponents *components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:NO];
  NSString *bundle = nil;
  NSMutableArray<NSURLQueryItem *> *remaining = [NSMutableArray array];
  for (NSURLQueryItem *item in components.queryItems) {
    if ([item.name isEqualToString:@"bundle"]) {
      bundle = item.value;
    } else {
      [remaining addObject:item];
    }
  }
  if (bundle.length == 0) {
    return nil;
  }
  NSMutableString *legacyUrl = [NSMutableString stringWithFormat:@"%@%@", LOCAL_URL_PREFIX, bundle];
  if (remaining.count > 0) {
    NSMutableArray<NSString *> *pairs = [NSMutableArray array];
    for (NSURLQueryItem *item in remaining) {
      [pairs addObject:[NSString stringWithFormat:@"%@=%@", item.name, item.value ?: @""]];
    }
    [legacyUrl appendFormat:@"?%@", [pairs componentsJoinedByString:@"&"]];
  }
  return legacyUrl;
}

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

  // On cold launch via deeplink, open the deeplink target instead of homepage.
  NSURL *launchUrl = launchOptions[UIApplicationLaunchOptionsURLKey];
  NSString *deeplinkTarget = launchUrl ? [AppDelegate legacyUrlFromDeeplink:launchUrl] : nil;
  if (deeplinkTarget) {
    deeplinkTarget = [AppDelegate appendThemeParams:deeplinkTarget];
  }

  // On cold launch via Quick Action, open the shortcut target instead of homepage.
  UIApplicationShortcutItem *shortcut = launchOptions[UIApplicationLaunchOptionsShortcutItemKey];
  if (!deeplinkTarget && shortcut) {
    deeplinkTarget = [self urlForShortcutItem:shortcut];
  }

  [[TasmDispatcher sharedInstance] openTargetUrl:deeplinkTarget ?: HOMEPAGE_URL];
  // Return NO when launched via shortcut to prevent performActionForShortcutItem:
  // from being called redundantly.
  return (shortcut == nil);
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

  // Handle lynx://lynxview_page?bundle=<name> deeplinks.
  NSString *deeplinkTarget = [AppDelegate legacyUrlFromDeeplink:url];
  if (deeplinkTarget) {
    [self openCard:[AppDelegate appendThemeParams:deeplinkTarget]];
    return YES;
  }

  return NO;
}

- (NSString *)urlForShortcutItem:(UIApplicationShortcutItem *)item {
  if ([item.type isEqualToString:@"com.lynx.explorer.showcase"]) {
    return [AppDelegate appendThemeParams:@"file://lynx?local://showcase/menu/main.lynx.bundle"];
  }
  return nil;
}

- (void)application:(UIApplication *)application
    performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem
               completionHandler:(void (^)(BOOL))completionHandler {
  NSString *url = [self urlForShortcutItem:shortcutItem];
  if (url) {
    [self openCard:url];
    completionHandler(YES);
    return;
  }
  completionHandler(NO);
}

@end
