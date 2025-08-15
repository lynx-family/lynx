// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxLogBoxWrapper.h"

#if OS_OSX
#import <Lynx/DevToolLogBoxHelper.h>
#import <Lynx/DevToolLogBoxProxy.h>
#else
#import <BaseDevtool/DevToolLogBoxHelper.h>
#import <BaseDevtool/DevToolLogBoxProxy.h>
#import <Lynx/LynxDevtool.h>
#endif
#import <Lynx/LynxLog.h>
#import <Lynx/LynxView+Internal.h>

NSString* const ERR_NAMESPACE = @"lynx";

@implementation LynxLogBoxWrapper {
  DevToolLogBoxProxy* _logBoxProxy;
  __weak LynxView* _lynxView;
  __weak UIViewController* _pageViewController;
#if OS_IOS
  __weak LynxDevtool* _devtool;
#endif
  NSString* _url;
}

- (instancetype)initWithLynxView:(LynxView*)view {
  LLogInfo(@"LynxLogBoxWrapper: initWithLynxView: %p", view);
  self = [super init];
  if (self) {
    _lynxView = view;
    _logBoxProxy = [[DevToolLogBoxProxy alloc] initWithNamespace:ERR_NAMESPACE
                                                resourceProvider:self];
    [_logBoxProxy registerErrorParserWithBundle:@"LynxDebugResources"
                                           file:@"logbox/lynx-error-parser"];
  }
  return self;
}

#pragma mark - LynxLogBoxProtocol

#if OS_IOS
- (void)setLynxDevTool:(LynxDevtool*)devtool {
  _devtool = devtool;
}
#endif

- (void)showLogMessage:(LynxError*)error {
  if (!error) {
    LLogInfo(@"LynxLogBoxWrapper: error is nil");
    return;
  }
  NSString* message = [error.userInfo objectForKey:LynxErrorUserInfoKeyMessage];
  [self sendErrorEventToPerf:message];
  [_logBoxProxy showLogMessage:message withLevel:error.level];
}

- (void)onMovedToWindow {
  [_logBoxProxy onMovedToWindow];
}

- (void)attachLynxView:(nonnull LynxView*)lynxView {
  _lynxView = lynxView;
  [_logBoxProxy onResourceProviderReady];
}

- (void)onLynxViewReload {
  [_logBoxProxy reset];
}

- (void)sendErrorEventToPerf:(NSString*)message {
#if OS_IOS
  __strong typeof(_lynxView) lynxView = _lynxView;
  if (!lynxView) {
    return;
  }
  NSDictionary* eventData = @{@"error" : message};
  [[[lynxView templateRender] devTool] onPerfMetricsEvent:@"lynx_error_event" withData:eventData];
#endif
}

- (void)destroy {
  [_logBoxProxy destroy];
}

#pragma mark - DevToolLogBoxResProvider

- (NSString*)entryUrlForLogSrc {
  __strong typeof(_lynxView) lynxView = _lynxView;
  LLogInfo(@"LynxLogBoxWrapper: proxy getTemplateUrl view: %p, url: %@", _lynxView, [lynxView url]);
  return _url ? _url : [lynxView url];
}

- (UIView*)getView {
  return _lynxView;
}

- (NSDictionary*)logSources {
  __strong typeof(_lynxView) lynxView = _lynxView;
  return [lynxView getAllJsSource];
}

- (NSString*)logSourceWithFileName:(NSString*)fileName {
#if OS_IOS
  if ([fileName hasSuffix:@"main-thread.js"]) {
    __strong typeof(_devtool) devtool = _devtool;
    if (devtool != nil) {
      return [devtool debugInfoUrl:fileName];
    }
  } else {
#endif
    NSDictionary* logSources = [self logSources];
    NSString* value = @"";
    int matchLength = 0;
    for (NSString* key in logSources) {
      if ([fileName hasSuffix:key] && key.length > matchLength) {
        matchLength = key.length;
        value = logSources[key];
      }
    }
    return value;
#if OS_IOS
  }
  return @"";
#endif
}

@end
