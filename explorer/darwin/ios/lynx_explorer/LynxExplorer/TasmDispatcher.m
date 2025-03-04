// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TasmDispatcher.h"
#import "AppDelegate.h"
#import "DemoTemplateResourceFetcher.h"
#import "LynxViewShellViewController.h"

@implementation TasmDispatcher {
  NSString *_latestQuery;
  NSMutableDictionary *_latestParams;
}

static TasmDispatcher *_instance = nil;
static NSMapTable<NSString *, __kindof UIViewController *> *_dispatchedViewControllers = nil;

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[self alloc] init];
    _dispatchedViewControllers = [NSMapTable weakToWeakObjectsMapTable];
  });
  return _instance;
}

- (void)generateLatestParams {
  if (_latestQuery == nil) {
    _latestParams = nil;
    return;
  }
  _latestParams = [[NSMutableDictionary alloc] init];
  for (NSString *param in [_latestQuery componentsSeparatedByString:@"&"]) {
    NSArray *elts = [param componentsSeparatedByString:@"="];
    if ([elts count] < 2) continue;
    [_latestParams setObject:[elts lastObject] forKey:[elts firstObject]];
  }
}

- (void)openTargetUrlSingleTop:(NSString *)sourceUrl {
  UINavigationController *vc =
      ((AppDelegate *)([UIApplication sharedApplication].delegate)).navigationController;
  [vc popViewControllerAnimated:NO];
  [self openTargetUrl:sourceUrl];
}

// sourceUrl: file://lynx?local://homepage.lynx.bundle
// processedUrl: local://homepage.lynx.bundle
- (void)openTargetUrl:(NSString *)sourceUrl {
  NSData *data = nil;
  NSString *url = nil;

  LocalBundleResult localRes = [DemoTemplateResourceFetcher readLocalBundleFromResource:sourceUrl];
  if (localRes.isLocalScheme) {
    data = localRes.data;
    url = localRes.url;
    _latestQuery = localRes.query;
  } else {
    NSURL *source = [NSURL URLWithString:sourceUrl];
    if ([source.scheme isEqualToString:@"http"] || [source.scheme isEqualToString:@"https"]) {
      _latestQuery = source.query;
      url = sourceUrl;
    }
  }

  [self generateLatestParams];

  BOOL animated = YES;
  if ([[_latestParams allKeys] containsObject:@"animated"]) {
    animated = [[_latestParams objectForKey:@"animated"] boolValue];
  }
  dispatch_async(dispatch_get_main_queue(), ^{
    UINavigationController *vc =
        ((AppDelegate *)([UIApplication sharedApplication].delegate)).navigationController;

    LynxViewShellViewController *shellVC = [LynxViewShellViewController new];
    shellVC.navigationController = (UINavigationController *)vc;
    shellVC.url = url;
    shellVC.data = data;
    shellVC.params = self->_latestParams;
    [vc pushViewController:shellVC animated:animated];
  });
}

@end
