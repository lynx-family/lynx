// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <DebugRouter/DebugRouter.h>

#import "AppDelegate+DebugRouter.h"
#import "DemoTemplateResourceFetcher.h"
#import "TasmDispatcher.h"

NSString *const DEBUG_ROUTER_APP_OPEN_PAGE = @"App.openPage";
NSString *const DEBUG_ROUTER_APP_CLOSE_PAGE = @"App.closePage";

static NSInteger const kDebugRouterAppActionSuccessCode = 0;
static NSInteger const kDebugRouterAppActionFailedCode = -1;

static NSMutableDictionary<NSString *, id> *LynxExplorerDebugRouterResponse(NSInteger code,
                                                                            NSString *message) {
  NSMutableDictionary<NSString *, id> *response = [[NSMutableDictionary alloc] initWithDictionary:@{
    @"code" : @(code)
  }];
  if (message.length > 0) {
    response[@"message"] = message;
  }
  return response;
}

static DebugRouterMessageHandleResult *LynxExplorerDebugRouterHandleResultFromResponse(
    NSDictionary<NSString *, id> *response) {
  NSInteger code = [response[@"code"] integerValue];
  NSString *message = response[@"message"];
  if (code == kDebugRouterAppActionSuccessCode) {
    return [[DebugRouterMessageHandleResult alloc] init];
  }
  return [[DebugRouterMessageHandleResult alloc] initWithCode:(int)code message:message ?: @""];
}

static void LynxExplorerRunOnMainThreadSync(dispatch_block_t block) {
  if (NSThread.isMainThread) {
    block();
    return;
  }
  dispatch_sync(dispatch_get_main_queue(), block);
}

@interface LynxExplorerOpenPageMessageHandler : NSObject <DebugRouterMessageHandler>

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate;

@end

@interface LynxExplorerClosePageMessageHandler : NSObject <DebugRouterMessageHandler>

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate;

@end

@implementation LynxExplorerOpenPageMessageHandler {
  __weak AppDelegate *_appDelegate;
}

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate {
  self = [super init];
  if (self) {
    _appDelegate = appDelegate;
  }
  return self;
}

- (nonnull NSString *)getName {
  return DEBUG_ROUTER_APP_OPEN_PAGE;
}

- (nonnull DebugRouterMessageHandleResult *)handleMessageWithParams:
    (NSMutableDictionary<NSString *, NSString *> *)params {
  return LynxExplorerDebugRouterHandleResultFromResponse(
      [_appDelegate debugRouterOpenPageResponseForURL:params[@"url"]]);
}

@end

@implementation LynxExplorerClosePageMessageHandler {
  __weak AppDelegate *_appDelegate;
}

- (instancetype)initWithAppDelegate:(AppDelegate *)appDelegate {
  self = [super init];
  if (self) {
    _appDelegate = appDelegate;
  }
  return self;
}

- (nonnull NSString *)getName {
  return DEBUG_ROUTER_APP_CLOSE_PAGE;
}

- (nonnull DebugRouterMessageHandleResult *)handleMessageWithParams:
    (NSMutableDictionary<NSString *, NSString *> *)params {
  return LynxExplorerDebugRouterHandleResultFromResponse(
      [_appDelegate debugRouterClosePageResponse]);
}

@end

@implementation AppDelegate (DebugRouter)

- (void)registerDebugRouterMessageHandlers {
  id<DebugRouterMessageHandler> openPageHandler =
      [[LynxExplorerOpenPageMessageHandler alloc] initWithAppDelegate:self];
  id<DebugRouterMessageHandler> closePageHandler =
      [[LynxExplorerClosePageMessageHandler alloc] initWithAppDelegate:self];
  [[DebugRouter instance] addMessageHandler:openPageHandler];
  [[DebugRouter instance] addMessageHandler:closePageHandler];
}

- (BOOL)isDebugRouterSupportedURL:(NSString *)url {
  if (url.length == 0) {
    return NO;
  }

  LocalBundleResult localResult = [DemoTemplateResourceFetcher readLocalBundleFromResource:url];
  if (localResult.isLocalScheme || localResult.isLynxRecorderSchema) {
    return YES;
  }

  NSString *encodedURL =
      [url stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet
                                                                  URLFragmentAllowedCharacterSet]];
  NSURL *parsedURL = [NSURL URLWithString:encodedURL];
  NSString *scheme = parsedURL.scheme.lowercaseString;
  return [scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"];
}

- (NSMutableDictionary<NSString *, id> *)debugRouterOpenPageResponseForURL:(NSString *)url {
  if (url.length == 0) {
    return LynxExplorerDebugRouterResponse(kDebugRouterAppActionFailedCode, @"url is required");
  }
  if (![self isDebugRouterSupportedURL:url]) {
    return LynxExplorerDebugRouterResponse(kDebugRouterAppActionFailedCode, @"unsupported url");
  }

  LynxExplorerRunOnMainThreadSync(^{
    [[TasmDispatcher sharedInstance] openTargetUrl:url];
  });
  return LynxExplorerDebugRouterResponse(kDebugRouterAppActionSuccessCode, nil);
}

- (NSMutableDictionary<NSString *, id> *)debugRouterClosePageResponse {
  __block NSMutableDictionary<NSString *, id> *response = nil;
  LynxExplorerRunOnMainThreadSync(^{
    if (self.navigationController.viewControllers.count <= 1) {
      response =
          LynxExplorerDebugRouterResponse(kDebugRouterAppActionFailedCode, @"no page to close");
      return;
    }
    [self.navigationController popViewControllerAnimated:YES];
    response = LynxExplorerDebugRouterResponse(kDebugRouterAppActionSuccessCode, nil);
  });
  return response;
}

@end
