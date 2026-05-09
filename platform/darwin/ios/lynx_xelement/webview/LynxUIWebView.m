// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropertyDiffMap.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxUIWebView.h>

@implementation LynxWebViewService

+ (instancetype)sharedInstance {
  static dispatch_once_t once;
  static id instance;
  dispatch_once(&once, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (NSMutableDictionary<NSString *, id<LynxWebViewLoaderProvider>> *)providers {
  if (!_providers) {
    _providers = [NSMutableDictionary dictionary];
  }
  return _providers;
}

- (id<LynxWebViewLoader> _Nullable)createLynxWebViewLoaderWithKey:(nonnull NSString *)key
                                                         delegate:(id<LynxWebViewLoaderDelegate>)
                                                                      delegate {
  return [self.providers[key] createLynxWebViewLoaderWithDelegate:delegate];
}

@end

@interface LynxWebViewWrapper : UIView

@end

@implementation LynxWebViewWrapper

- (void)layoutSubviews {
  [super layoutSubviews];
  [self.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    obj.frame = self.bounds;
  }];
}

@end

@interface LynxUIWebView () <LynxWebViewLoaderDelegate>
@property(nonatomic, assign) BOOL urlExists;
@property(nonatomic, strong) id<LynxWebViewLoader> loader;
@property(nonatomic, strong) NSString *loaderType;
@property(nonatomic, strong) LynxPropertyDiffMap *diffMap;
@end

@implementation LynxUIWebView

- (UIView *)createView {
  self.firstRender = YES;
  self.diffMap = [[LynxPropertyDiffMap alloc] init];
  [self.diffMap putValue:@(YES) forKey:@"bounces"];
  [self.diffMap putValue:@(NO) forKey:@"scroll-bar-enable"];
  [self.diffMap putValue:@(NO) forKey:@"enable-debug"];
  self.loaderType = @"default";
  return [[LynxWebViewWrapper alloc] init];
}

LYNX_PROP_SETTER("bounces", bounces, BOOL) { [self.diffMap putValue:@(value) forKey:@"bounces"]; }

LYNX_PROP_SETTER("scroll-bar-enable", setScrollBarEnable, BOOL) {
  [self.diffMap putValue:@(value) forKey:@"scroll-bar-enable"];
}

LYNX_PROP_SETTER("params", setParams, NSDictionary *) {
  [self.diffMap putValue:value ?: @{} forKey:@"params"];
}

LYNX_PROP_SETTER("src", src, NSString *) { [self.diffMap putValue:value ?: @"" forKey:@"src"]; }

LYNX_PROP_SETTER("html", html, NSString *) { [self.diffMap putValue:value ?: @"" forKey:@"html"]; }

LYNX_PROP_SETTER("enable-debug", setEnableDebug, BOOL) {
  [self.diffMap putValue:@(value) forKey:@"enable-debug"];
}

LYNX_PROP_SETTER("webview-type", setWebViewType, NSString *) { self.loaderType = value; }

LYNX_UI_METHOD(eval) {
  NSString *jsCode = params[@"func"];
  if (jsCode) {
    [self.loader evaluateJavaScript:jsCode
                  completionHandler:^(id _Nullable object, NSError *_Nullable error) {
                    if (callback) {
                      if (error) {
                        callback(kUIMethodOperationError, error.description ?: @"unknown");
                      } else {
                        callback(kUIMethodSuccess, nil);
                      }
                    }
                  }];
  } else {
    if (callback) {
      callback(kUIMethodParamInvalid, @"invalid func");
    }
  }
}

LYNX_UI_METHOD(reload) {
  if (callback) {
    if (self.urlExists) {
      [self.loader reload];
      callback(kUIMethodSuccess, nil);
    } else {
      callback(kUIMethodInvalidStateError, @"invalid src");
    }
  }
}

- (void)onNodeReady {
  [super onNodeReady];
  if (self.firstRender) {
    self.firstRender = NO;
    self.loader =
        [[LynxWebViewService sharedInstance] createLynxWebViewLoaderWithKey:self.loaderType
                                                                   delegate:self];
    if (!self.loader) {
      NSString *tagName = self.tagName ?: @"webview";
      @throw [NSException
          exceptionWithName:@"LynxCreateUIException"
                     reason:[NSString stringWithFormat:@"There are no implementation for `%@` with "
                                                       @"type-\"%@\", please inject it at Native",
                                                       tagName, self.loaderType]
                   userInfo:nil];
    }
    [self.loader setParams:[self.diffMap getValueForKey:@"params"]];
    [self ensureWebView];
  } else {
    NSDictionary *params = nil;
    if ([self.diffMap valueChangedForKey:@"params" updateTo:&params]) {
      [self.loader setParams:params];
    }
  }

  NSString *url = nil;
  NSString *html = nil;
  BOOL urlChanged = [self.diffMap valueChangedForKey:@"src" setTo:&url];
  BOOL htmlChanged = [self.diffMap valueChangedForKey:@"html" setTo:&html];

  if (urlChanged || htmlChanged) {
    if (url.length && urlChanged) {
      self.urlExists = YES;
      [self.loader load:url];
    } else if (html.length && !url.length) {
      self.urlExists = YES;
      [self.loader loadHtmlString:html];
    }
    if (!url.length && !html.length) {
      self.urlExists = NO;
      [self.context.eventEmitter
          sendCustomEvent:[[LynxDetailEvent alloc]
                              initWithName:@"error"
                                targetSign:[self sign]
                                    detail:@{
                                      @"errorCode" : @-1,
                                      @"errorMsg" : @"invalid input: src and html "
                                                    @"are empty"
                                    }]];
    }
  }

  [self ensureWebView];

  [self.diffMap clearDirtyRecords];
}

- (void)ensureWebView {
  // For some impl, the webview must be created while a proper url is loaded.
  UIView *container = [self.loader getContainerView];
  if (container) {
    if (container.superview != self.view) {
      [self.view addSubview:container];
    }
    container.backgroundColor = [UIColor clearColor];

    WKWebView *webView = [self.loader getWebView];
    webView.opaque = NO;
    webView.backgroundColor = [UIColor clearColor];
    webView.scrollView.backgroundColor = [UIColor clearColor];

    BOOL bounces = [[self.diffMap getValueForKey:@"bounces"] boolValue];
    webView.scrollView.bounces = bounces;

    BOOL scrollBarEnable = [[self.diffMap getValueForKey:@"scroll-bar-enable"] boolValue];
    webView.scrollView.showsVerticalScrollIndicator = scrollBarEnable;
    webView.scrollView.showsHorizontalScrollIndicator = scrollBarEnable;

    if (@available(iOS 16.4, *)) {
      webView.inspectable = [[self.diffMap getValueForKey:@"enable-debug"] boolValue];
    }
  }
}

- (NSString *)nameOfScriptMessageHandler {
  return @"nativeApp";
}

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
  id msg = message.body;
  if ([NSJSONSerialization isValidJSONObject:msg]) {
    NSData *data = [NSJSONSerialization dataWithJSONObject:msg options:0 error:nil];
    if (data) {
      msg = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    }
  }
  if (![msg isKindOfClass:[NSString class]]) {
    msg = [NSString stringWithFormat:@"%@", msg ?: @"unknown"];
  }

  [self.context.eventEmitter
      sendCustomEvent:[[LynxDetailEvent alloc] initWithName:@"message"
                                                 targetSign:[self sign]
                                                     detail:@{@"msg" : msg}]];
  [self enableTapGestureSimultaneouslyRecursively:self.loader.getWebView];
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
  [self.loader evaluateJavaScript:@"(function() { window.addEventListener('message', "
                                  @"function(event) { window.webkit.messageHandlers."
                                  @"nativeApp.postMessage(event.data);}); })();"
                completionHandler:nil];

  [self.context.eventEmitter sendCustomEvent:[[LynxDetailEvent alloc] initWithName:@"load"
                                                                        targetSign:[self sign]
                                                                            detail:nil]];
  [self enableTapGestureSimultaneouslyRecursively:webView];
}

- (void)webView:(WKWebView *)webView
    didFailNavigation:(WKNavigation *)navigation
            withError:(NSError *)error {
  [self.context.eventEmitter
      sendCustomEvent:[[LynxDetailEvent alloc] initWithName:@"error"
                                                 targetSign:[self sign]
                                                     detail:@{
                                                       @"errCode" : @(error.code),
                                                       @"errMsg" : error.description ?: @"unknown"
                                                     }]];
  [self enableTapGestureSimultaneouslyRecursively:webView];
}

- (void)enableTapGestureSimultaneouslyRecursively:(UIView *)view {
  view.lynxEnableTapGestureSimultaneously = YES;
  [view.subviews enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    [self enableTapGestureSimultaneouslyRecursively:obj];
  }];
}

@end
