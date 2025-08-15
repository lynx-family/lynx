// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "DevToolLogBox.h"

#if OS_OSX
#import <Lynx/DevToolDownloader.h>
#import <Lynx/DevToolToast.h>
#else
#import <BaseDevtool/DevToolDownloader.h>
#import <BaseDevtool/DevToolToast.h>
#endif
#import <TargetConditionals.h>
#import <WebKit/WebKit.h>
#import "DevToolLogBoxEnv.h"

#if OS_OSX
#define DevToolLogBoxBaseWindow NSView
#else
#define DevToolLogBoxBaseWindow UIWindow
#endif
@class DevToolLogBoxWindow;

typedef void (^JsApiHandler)(NSDictionary *, NSNumber *);
CGFloat const kContentHeightPercent = 0.6;
NSDictionary *const kLevelDic = @{
  @"-1" : @"verbose",
  @"0" : @"info",
  @"1" : @"warning",
  @"2" : @"error",
  @"3" : @"error",
  @"4" : @"info",
  @"5" : @"error"
};
NSString *const BRIDGE_JS =
    @"(function () {"
     "var id = 0, callbacks = {}, eventListeners = {};"
     "var nativeBridge = window.nativeBridge || window.webkit.messageHandlers.nativeBridge;"
     "window.logbox = {"
     "  call: function(bridgeName, callback, data) {"
     "    var thisId = id++;"
     "    callbacks[thisId] = callback;"
     "    nativeBridge.postMessage({"
     "      bridgeName: bridgeName,"
     "      data: data || {},"
     "      callbackId: thisId"
     "    });"
     "  },"
     "  on: function(event, handler) {"
     "    eventListeners[event] = handler;"
     "  },"
     "  sendResult: function(msg) {"
     "    var callbackId = msg.callbackId;"
     "    if (callbacks[callbackId]) {"
     "      callbacks[callbackId](msg.data);"
     "    }"
     "  },"
     "  sendEvent: function(msg) {"
     "    if (eventListeners[msg.event]) {"
     "      eventListeners[msg.event](msg.data);"
     "    }"
     "  }"
     "};"
     "})();"
     "Object.defineProperty(navigator, 'userAgent', {"
     "  value: navigator.userAgent + ' Lynx LogBox',"
     "  writable: false"
     "});"
     "document.dispatchEvent(new Event('LogBoxReady'));";

#pragma mark - LogBoxCache
@interface DevToolLogBoxCache : NSObject

@property(nonatomic, readwrite) NSMutableArray *errorMessages;
@property(nonatomic, readwrite) NSString *errNamespace;
@property(nonatomic, readwrite) NSDictionary *logSources;

- (instancetype)init;

@end

@implementation DevToolLogBoxCache

- (instancetype)init {
  if (self = [super init]) {
    self.errorMessages = nil;
    self.logSources = nil;
  }
  return self;
}

@end

#pragma mark - DevToolLogBoxWindowActionDelegate
@protocol DevToolLogBoxWindowActionDelegate <NSObject>
@optional
// Only needed in DevToolLogBox
- (void)clearCurrentLogs;
@optional
- (void)changeView:(NSDictionary *)params;
@optional
- (void)dismiss;
- (NSDictionary *)getLogSources;
- (NSString *)getLogSourceWithFileName:(NSString *)fileName;
@end

#pragma mark - DevToolLogBoxWindow
@interface DevToolLogBoxWindow
    : DevToolLogBoxBaseWindow <WKScriptMessageHandler, WKNavigationDelegate>
@property(nonatomic, weak) id<DevToolLogBoxWindowActionDelegate> actionDelegate;
@property(nonatomic, copy) void (^loadingFinishCallback)();
@property(nonatomic, readwrite) DevToolLogBoxCache *logBoxCache;

- (BOOL)isShowing;
- (void)onLogBoxDestroy;
- (void)destroy;

@end

@implementation DevToolLogBoxWindow {
  WKWebView *_stackTraceWebView;
  NSDictionary *_jsAPI;
  NSString *_entryUrlForLogSrc;
  BOOL _isLogBoxDestroyed;
#if OS_OSX
  // click background to dismiss log box
  NSView *_clickGestureView;
#else
  __weak UIWindow *_previous_keywindow;
#endif
}

- (instancetype)initWithFrame:(CGRect)frame URL:(NSString *)entryUrlForLogSrc {
  if (self = [super initWithFrame:frame]) {
#if OS_IOS
    self.windowLevel = UIWindowLevelStatusBar - 1;
    self.backgroundColor = [UIColor clearColor];
    _previous_keywindow = nil;
#endif
    self.logBoxCache = nil;
    self.hidden = YES;
    _isLogBoxDestroyed = false;
    __weak typeof(self) weakSelf = self;
    // clang-format off
    // clang-format cannot format correctly here
    _jsAPI = @{
        @"deleteLynxview": ^(NSDictionary *params, NSNumber *callbackId) {
            [weakSelf clearCurrentLogs];
        },
        @"changeView": ^(NSDictionary *params, NSNumber *callbackId) {
          [weakSelf changeView:params];
        },
        @"dismiss": ^(NSDictionary *params, NSNumber *callbackId) {
          [weakSelf dismiss];
        },
        @"toast": ^(NSDictionary *params, NSNumber *callbackId) {
            NSString *message = params ? params[@"message"] : nil;
            [DevToolToast showToast:message];
        },
        @"queryResource": ^(NSDictionary *params, NSNumber *callbackId) {
            NSString *name = params ? params[@"name"] : nil;
            [weakSelf getResource:name withCallbackId:callbackId];
        },
        @"loadErrorParser": ^(NSDictionary *params, NSNumber *callbackId) {
            NSString* errNamespace = params ? params[@"namespace"] : nil;
           [weakSelf loadErrorParser:errNamespace withCallbackId:callbackId];
        },
    };
    _entryUrlForLogSrc = entryUrlForLogSrc;
#if OS_OSX
    // add gesture on subview
    _clickGestureView = [[NSView alloc] init];
    _clickGestureView.wantsLayer = YES;
    _clickGestureView.layer.backgroundColor = [NSColor colorWithWhite:0 alpha:0.8].CGColor;
    [self addSubview:_clickGestureView];
    NSClickGestureRecognizer *gestureRecognizer =
    [[NSClickGestureRecognizer alloc] initWithTarget:self action:@selector(dismiss)];
    [_clickGestureView addGestureRecognizer:gestureRecognizer];
#else
    UIViewController *rootController = [UIViewController new];
    self.rootViewController = rootController;
    UIView *rootView = rootController.view;
    rootView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.8];
    UITapGestureRecognizer *gestureRecognizer =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismiss)];
    [rootView addGestureRecognizer:gestureRecognizer];
#endif
    WKWebViewConfiguration *configuration = [[WKWebViewConfiguration alloc] init];
    configuration.userContentController = [[WKUserContentController alloc] init];
    [configuration.userContentController addScriptMessageHandler:self name:@"nativeBridge"];
    WKUserScript *bridgeScript =
        [[WKUserScript alloc] initWithSource:BRIDGE_JS
                               injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                            forMainFrameOnly:YES];
    [configuration.userContentController addUserScript:bridgeScript];
    _stackTraceWebView = [[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration];
    _stackTraceWebView.navigationDelegate = self;
#if OS_OSX
    [self addSubview:_stackTraceWebView];
#else
    [rootView addSubview:_stackTraceWebView];
#endif
      NSURL *url = [self getLogBoxPageUrl];
      if (url) {
          [_stackTraceWebView loadFileURL:url allowingReadAccessToURL:url];
          [self resizeWindow];
      } else {
          NSLog(@"get log box page url failed");
      }
#if OS_OSX
/**
 * When lynx view destroyed, DevToolLogBoxWindow might still exist (#4780).
 * At the moment we reload lynx view, we need remove previous window (NSView).
 */
    for (NSView *window in [NSApplication sharedApplication]
             .mainWindow.contentView.subviews.reverseObjectEnumerator) {
      if ([window isKindOfClass:[DevToolLogBoxWindow class]]) {
        if (((DevToolLogBoxWindow *)window)->_isLogBoxDestroyed) {
          [(DevToolLogBoxWindow *)window destroy];
          [window removeFromSuperview];
        }
      }
    }
    [[NSApplication sharedApplication].mainWindow.contentView addSubview:self];
    // nofitication center contains weak reference of self
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowDidResize:)
                                                 name:NSWindowDidResizeNotification
                                               object:[NSApplication sharedApplication].mainWindow];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowWillClose:)
                                                 name:NSWindowWillCloseNotification
                                               object:[NSApplication sharedApplication].mainWindow];
#endif
  }
  return self;
}
// clang-format on

#if OS_OSX
// avoid memory leak when window closed while log box still showing
- (void)windowWillClose:(NSNotification *)notification {
  // LogBox dealloc before window close
  if (self->_isLogBoxDestroyed) {
    [self destroy];
  } else if (!self.hidden) {
    // LogBox still exists, it will destroy window when dealloc but window needs be hidden first
    self.hidden = YES;
  }
}

- (void)windowDidResize:(NSNotification *)notification {
  // when log box is hidden, don't need to resize window
  if (!self.hidden) {
    [self resizeWindow];
  }
}
#endif

- (void)resizeWindow {
#if OS_OSX
  [self setFrame:[NSApplication sharedApplication].mainWindow.contentView.frame];
  CGRect clickFrame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
  [_clickGestureView setFrame:NSRectFromCGRect(clickFrame)];
  CGRect webViewFrame =
      CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height * kContentHeightPercent);
  [_stackTraceWebView setFrame:NSRectFromCGRect(webViewFrame)];
#else
  UIView *rootView = self.rootViewController.view;
  CGRect webViewFrame = CGRectMake(0, rootView.bounds.size.height, rootView.bounds.size.width,
                                   rootView.bounds.size.height * kContentHeightPercent);
  [_stackTraceWebView setFrame:webViewFrame];
#endif
}

- (NSURL *)getLogBoxPageUrl {
  NSURL *url;
#if OS_OSX
  NSURL *debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                  withExtension:@"bundle"];
#else
  NSURL *debugBundleUrl =
      [[NSBundle bundleForClass:[self class]] URLForResource:@"LynxBaseDevToolResources"
                                               withExtension:@"bundle"];
#endif
  if (debugBundleUrl) {
    NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
    url = [bundle URLForResource:@"logbox/index" withExtension:@".html"];
    if (!url) {
      return nil;
    }
  }
  return url;
}

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
  if ([message.name isEqualToString:@"nativeBridge"]) {
    if ([message.body isKindOfClass:[NSDictionary class]]) {
      NSString *bridgeName = [message.body objectForKey:@"bridgeName"];
      NSDictionary *data = [message.body objectForKey:@"data"];
      NSNumber *callbackId = [message.body objectForKey:@"callbackId"];
      if (bridgeName) {
        JsApiHandler handler = _jsAPI[bridgeName];
        if (handler) {
          handler(data, callbackId);
        } else {
          NSLog(@"Unknown JSAPI");
        }
      }
    }
  }
}

- (void)destroy {
  [_stackTraceWebView.configuration.userContentController
      removeScriptMessageHandlerForName:@"nativeBridge"];
}

#pragma mark - Helper

- (NSString *)dict2JsonString:(NSDictionary *)dict {
  NSData *json = [NSJSONSerialization dataWithJSONObject:dict
                                                 options:NSJSONWritingPrettyPrinted
                                                   error:nil];
  NSString *str = [[NSString alloc] initWithData:json encoding:NSUTF8StringEncoding];
  NSMutableString *mutStr = [NSMutableString stringWithString:str];
  [mutStr replaceOccurrencesOfString:@"\n"
                          withString:@""
                             options:NSLiteralSearch
                               range:{0, str.length}];
  return mutStr;
}

- (void)sendJsResult:(NSDictionary *)msg {
  NSString *js =
      [NSString stringWithFormat:@"window.logbox.sendResult(%@);", [self dict2JsonString:msg]];
  [self evaluateJS:js];
}

- (void)evaluateJS:(NSString *)js {
  if (!js) {
    NSLog(@"Failed to evaluate the script, the js is nil");
    return;
  }
  [_stackTraceWebView evaluateJavaScript:js
                       completionHandler:^(id _Nullable result, NSError *_Nullable error) {
                         if (error) {
                           NSLog(@"%@", [error localizedDescription]);
                         }
                       }];
}

- (void)sendJsEvent:(NSDictionary *)msg {
  NSString *js =
      [NSString stringWithFormat:@"window.logbox.sendEvent(%@);", [self dict2JsonString:msg]];
  [_stackTraceWebView evaluateJavaScript:js completionHandler:nil];
}

- (NSMutableDictionary *)normalizeMessage:(NSDictionary *)message {
  NSMutableDictionary *mutableMessage = [message mutableCopy];
  NSString *level = [kLevelDic objectForKey:message[@"level"]];
  if (!level) level = @"info";
  [mutableMessage setObject:level forKey:@"level"];
  return mutableMessage;
}

#pragma mark - Public Method

- (void)showLogMessage:(NSString *)message withNamespace:(NSString *)errNamespace {
  [self sendJsEvent:@{
    @"event" : @"receiveNewLog",
    @"data" : @{@"log" : message, @"namespace" : errNamespace}
  }];
}

- (void)updateViewInfo:(NSDictionary *)viewInfo {
  [self sendJsEvent:@{@"event" : @"receiveViewInfo", @"data" : viewInfo}];
}

- (void)show {
  if (self.isHidden) {
#if OS_OSX
    // resize first because window size might change when _window dismiss
    [self resizeWindow];
    self.hidden = NO;
    CGRect startAnimationFrame =
        CGRectMake(0, -self.bounds.size.height * kContentHeightPercent, self.frame.size.width,
                   self.frame.size.height * kContentHeightPercent);
    self->_stackTraceWebView.animator.frame = startAnimationFrame;
    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *_Nonnull context) {
      context.duration = 0.3;
      CGRect endAnimationFrame =
          CGRectMake(0, 0, self.frame.size.width, self.frame.size.height * kContentHeightPercent);
      self->_stackTraceWebView.animator.frame = endAnimationFrame;
    }];
#else
    _previous_keywindow = [self getKeyWindow];
    if (@available(iOS 13.0, *)) {
      for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if (scene.activationState == UISceneActivationStateForegroundActive &&
            [scene isKindOfClass:[UIWindowScene class]]) {
          self.windowScene = (UIWindowScene *)scene;
          break;
        }
      }
    }
    [self makeKeyAndVisible];
    CGFloat rootViewHeight = self.rootViewController.view.bounds.size.height;
    [UIView animateWithDuration:0.3
                     animations:^{
                       CGRect frame = CGRectMake(0, rootViewHeight * (1 - kContentHeightPercent),
                                                 [UIScreen mainScreen].bounds.size.width,
                                                 rootViewHeight * kContentHeightPercent);
                       [self->_stackTraceWebView setFrame:frame];
                     }];
#endif
  }
}

- (void)dismiss {
#if OS_OSX
  CGRect startAnimationFrame =
      CGRectMake(0, 0, self.frame.size.width, self.frame.size.height * kContentHeightPercent);
  self->_stackTraceWebView.animator.frame = startAnimationFrame;
  [NSAnimationContext
      runAnimationGroup:^(NSAnimationContext *_Nonnull context) {
        context.duration = 0.3;
        CGRect endAnimationFrame =
            CGRectMake(0, -self.bounds.size.height * kContentHeightPercent, self.frame.size.width,
                       self.frame.size.height * kContentHeightPercent);
        self->_stackTraceWebView.animator.frame = endAnimationFrame;
      }
      completionHandler:^{
        [self sendJsEvent:@{@"event" : @"reset"}];
        self.hidden = YES;
      }];
#else
  CGFloat rootViewHeight = self.rootViewController.view.bounds.size.height;
  [UIView animateWithDuration:0.3
      animations:^{
        CGRect frame = CGRectMake(0, rootViewHeight, [UIScreen mainScreen].bounds.size.width,
                                  rootViewHeight * kContentHeightPercent);
        [self->_stackTraceWebView setFrame:frame];
      }
      completion:^(BOOL finished) {
        [self sendJsEvent:@{@"event" : @"reset"}];
        self.hidden = YES;
        // restore previous key window when dismiss
        __strong typeof(_previous_keywindow) previous_keywindow = self->_previous_keywindow;
        [previous_keywindow makeKeyWindow];
        self->_previous_keywindow = nil;
      }];
#endif
  if (_actionDelegate && [_actionDelegate respondsToSelector:@selector(dismiss)]) {
    [_actionDelegate dismiss];
  }
  if (_isLogBoxDestroyed) {
    [self destroy];
  }
}

- (void)clearCurrentLogs {
  if (_actionDelegate && [_actionDelegate respondsToSelector:@selector(clearCurrentLogs)]) {
    [_actionDelegate clearCurrentLogs];
  }
}

- (void)changeView:(NSDictionary *)params {
  if (_actionDelegate && [_actionDelegate respondsToSelector:@selector(changeView:)]) {
    [_actionDelegate changeView:params];
  }
}

- (void)download:(NSString *)url withCallbackId:(NSNumber *)callbackId {
  if (url) {
    [DevToolDownloader
            download:url
        withCallback:^(NSData *_Nullable data, NSError *_Nullable error) {
          NSDictionary *msg = nil;
          if (!error) {
            NSString *content = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            msg = [NSDictionary
                dictionaryWithObjectsAndKeys:callbackId, @"callbackId", content, @"data", nil];
          } else {
            msg = [NSDictionary dictionaryWithObjectsAndKeys:callbackId, @"callbackId",
                                                             @"Download failed", @"data", nil];
          }
          dispatch_async(dispatch_get_main_queue(), ^{
            [self sendJsResult:msg];
          });
        }];
  } else {
    NSDictionary *msg = [NSDictionary
        dictionaryWithObjectsAndKeys:callbackId, @"callbackId", @"no url in params", @"data", nil];
    [self sendJsResult:msg];
  }
}

- (void)getResource:(NSString *)name withCallbackId:(NSNumber *)callbackId {
  NSDictionary *emptyData =
      [NSDictionary dictionaryWithObjectsAndKeys:callbackId, @"callbackId", @"", @"data", nil];
  if (!name) {
    [self sendJsResult:emptyData];
    return;
  }
  if ([name hasPrefix:@"http"]) {
    [self download:name withCallbackId:callbackId];
    return;
  }

  NSString *value = [_actionDelegate getLogSourceWithFileName:name];
  if (!value || !value.length) {
    NSDictionary *logSources = nil;
    if (!_isLogBoxDestroyed) {
      logSources = [_actionDelegate getLogSources];
    } else {
      logSources = _logBoxCache.logSources;
    }

    for (NSString *key in logSources) {
      if ([key containsString:name]) {
        value = logSources[key];
        break;
      }
    }
  } else if ([value hasPrefix:@"http"]) {
    [self download:value withCallbackId:callbackId];
    return;
  }
  NSDictionary *data = emptyData;
  if (value) {
    data =
        [NSDictionary dictionaryWithObjectsAndKeys:callbackId, @"callbackId", value, @"data", nil];
  }
  [self sendJsResult:data];
}

- (void)loadErrorParser:(NSString *)errNamespace withCallbackId:(NSNumber *)callbackId {
  [[DevToolLogBoxEnv sharedInstance]
      loadErrorParser:errNamespace
           completion:^(NSString *_Nullable data, NSError *_Nullable error) {
             if (!error) {
               [self evaluateJS:data];
               [self sendJsResult:@{@"callbackId" : callbackId, @"data" : @YES}];
             } else {
               [self sendJsResult:@{@"callbackId" : callbackId, @"data" : @NO}];
             }
           }];
}

- (BOOL)isShowing {
  return !self.isHidden;
}

- (void)onLogBoxDestroy {
  _isLogBoxDestroyed = true;
}

#if OS_IOS
- (UIWindow *)getKeyWindow {
  if (@available(iOS 15.0, *)) {
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
      if (scene.activationState == UISceneActivationStateForegroundActive &&
          [scene isKindOfClass:[UIWindowScene class]]) {
        return ((UIWindowScene *)scene).keyWindow;
      }
    }
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    for (UIWindow *window in [UIApplication sharedApplication].windows) {
#pragma clang diagnostic pop
      if (window.isKeyWindow) {
        return window;
      }
    }
  }
  return nil;
}
#endif

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
  if (!_isLogBoxDestroyed) {
    [self loadMappingsWasm];
    if (self.loadingFinishCallback) {
      self.loadingFinishCallback();
    }
  } else {
    [self.logBoxCache.errorMessages
        enumerateObjectsUsingBlock:^(NSString *_Nonnull message, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          [self showLogMessage:message withNamespace:self.logBoxCache.errNamespace];
        }];
  }
}

- (void)webView:(WKWebView *)webView
    didFailProvisionalNavigation:(WKNavigation *)navigation
                       withError:(NSError *)error {
  if (error && !_stackTraceWebView.isLoading) {
    NSLog(@"Load log box page failed: %@", [error localizedDescription]);
  }
}

- (void)webView:(WKWebView *)webView
    decidePolicyForNavigationResponse:(WKNavigationResponse *)navigationResponse
                      decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler {
  if ([navigationResponse.response isKindOfClass:[NSHTTPURLResponse class]]) {
    NSInteger statusCode = ((NSHTTPURLResponse *)navigationResponse.response).statusCode;
    if (statusCode / 100 == 4 || statusCode / 100 == 5) {
      decisionHandler(WKNavigationResponsePolicyCancel);
    } else {
      decisionHandler(WKNavigationResponsePolicyAllow);
    }
  } else {
    decisionHandler(WKNavigationResponsePolicyAllow);
  }
}

- (void)loadMappingsWasm {
#if OS_OSX
  NSURL *debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                  withExtension:@"bundle"];
#else
  NSURL *debugBundleUrl =
      [[NSBundle bundleForClass:[self class]] URLForResource:@"LynxBaseDevToolResources"
                                               withExtension:@"bundle"];
#endif
  if (!debugBundleUrl) {
    NSLog(@"Failed to load mappings.wasm: resource bundle not exists");
    return;
  }
  NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
  NSString *path = [bundle pathForResource:@"logbox/mappings" ofType:@"wasm"];
  if (!path) {
    NSLog(@"Failed to load mappings.wasm: file not exists");
    return;
  }
  NSData *fileData = [NSData dataWithContentsOfFile:path];
  if (!fileData) {
    NSLog(@"Failed to load mappings.wasm: can not load file content data");
    return;
  }
  NSString *base64Data = [fileData base64EncodedStringWithOptions:0];
  NSDictionary *event =
      @{@"event" : @"loadFile", @"data" : @{@"type" : @"mappings.wasm", @"data" : base64Data}};
  [self sendJsEvent:event];
}

@end

#pragma mark - DevToolLogBox
@interface DevToolLogBox () <DevToolLogBoxWindowActionDelegate>
@end

@implementation DevToolLogBox {
  __weak DevToolLogBoxManager *_manager;
  DevToolLogBoxWindow *_window;
  __weak DevToolLogBoxProxy *_currentProxy;
  NSString *_currentLevel;
  NSString *_entryUrlForLogSrc;
  // start from 1, send updateViewInfo to webview when it changes, default for -1.
  NSInteger _index;
  NSInteger _count;
  bool _isLoadingFinished;
}

- (instancetype)initWithLogBoxManager:(DevToolLogBoxManager *)manager {
  _window = nil;
  _manager = manager;
  _currentProxy = nil;
  _entryUrlForLogSrc = @"";
  _index = -1;
  _count = -1;
  _isLoadingFinished = false;
  return self;
}

- (void)updateViewInfo:(NSString *)url currentIndex:(NSInteger)index totalCount:(NSInteger)count {
  if ([NSThread isMainThread]) {
    [self updateViewInfoOnMainThread:url currentIndex:index totalCount:count];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf updateViewInfoOnMainThread:url currentIndex:index totalCount:count];
    });
  }
}

- (void)updateEntryUrlForLogSrc:(NSString *)url {
  if ([NSThread isMainThread]) {
    [self updateViewInfoOnMainThread:url currentIndex:_index totalCount:_count];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf updateViewInfoOnMainThread:url
                                currentIndex:strongSelf->_index
                                  totalCount:strongSelf->_count];
    });
  }
}

- (void)updateViewInfoOnMainThread:(NSString *)url
                      currentIndex:(NSInteger)index
                        totalCount:(NSInteger)count {
  NSLog(@"logbox: logbox updateViewInfoOnMainThread, url: %@", url);
  if (index != _index || count != _count || ![_entryUrlForLogSrc isEqualToString:url]) {
    _index = index;
    _count = count;
    _entryUrlForLogSrc = url != nil ? url : @"";
    if (!_isLoadingFinished) {
      return;
    }
    [self updateViewInfoOnWindow];
  }
}

- (void)updateViewInfoOnWindow {
  [self->_window updateViewInfo:@{
    @"currentView" : [NSNumber numberWithInteger:_index],
    @"viewsCount" : [NSNumber numberWithInteger:_count],
    @"level" : _currentLevel,
    @"templateUrl" : _entryUrlForLogSrc != nil ? _entryUrlForLogSrc : @""
  }];
}

- (BOOL)onNewLog:(NSString *)message
       withLevel:(NSString *)level
       withProxy:(DevToolLogBoxProxy *)proxy {
  if ([NSThread isMainThread]) {
    [self showLogMessageOnMainThread:message withLevel:level withProxy:proxy];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf showLogMessageOnMainThread:message withLevel:level withProxy:proxy];
    });
  }
  return _isLoadingFinished;
}

- (void)showLogMessageOnMainThread:(NSString *)message
                         withLevel:(NSString *)level
                         withProxy:(DevToolLogBoxProxy *)proxy {
  _currentProxy = proxy;
  _currentLevel = level;
  NSString *url = [proxy entryUrlForLogSrc];
  _entryUrlForLogSrc = url != nil ? url : @"";
  if (_isLoadingFinished) {
    [self showLogMessageOnWindow:message withNamespace:[proxy getErrorNamespace]];
  }
  [self showOnMainThread];
}

- (void)showLogMessageOnWindow:(NSString *)message withNamespace:(NSString *)errNamespace {
  [_window showLogMessage:message withNamespace:errNamespace];
}

- (void)showOnMainThread {
  if (!self->_window) {
    CGRect windowFrame;
#if TARGET_OS_OSX
    windowFrame = [NSApplication sharedApplication].mainWindow.frame;
#else
    windowFrame = [UIScreen mainScreen].bounds;
#endif
    self->_window = [[DevToolLogBoxWindow alloc] initWithFrame:windowFrame URL:_entryUrlForLogSrc];
    self->_window.actionDelegate = self;
    __weak __typeof(self) weakSelf = self;
    [self->_window setLoadingFinishCallback:^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      if (strongSelf) {
        strongSelf->_isLoadingFinished = true;
        [strongSelf updateViewInfoOnWindow];
        __weak typeof(strongSelf->_currentProxy) weakCurrentProxy = strongSelf->_currentProxy;
        __strong typeof(weakCurrentProxy) currentProxy = weakCurrentProxy;
        [[currentProxy logMessagesWithLevel:strongSelf->_currentLevel]
            enumerateObjectsUsingBlock:^(NSString *_Nonnull message, NSUInteger idx,
                                         BOOL *_Nonnull stop) {
              [strongSelf showLogMessageOnWindow:message
                                   withNamespace:[currentProxy getErrorNamespace]];
            }];
      }
    }];
  }
  [self->_window show];
}

- (BOOL)isShowing {
  return [_window isShowing];
}

- (NSString *)getCurrentLevel {
  return _currentLevel;
}

- (nullable DevToolLogBoxProxy *)getCurrentProxy {
  return _currentProxy;
}

- (void)clearCurrentLogs {
  __strong typeof(_manager) manager = _manager;
  [manager removeCurrentLogsWithLevel:_currentLevel];
}

- (void)dismissIfNeeded {
  if ([_window isShowing]) {
    [_window dismiss];
  }
}

- (void)dismiss {
  _index = -1;
  _count = -1;
  _entryUrlForLogSrc = @"";
}

- (void)changeView:(NSDictionary *)params {
  NSNumber *indexNum = [params objectForKey:@"viewNumber"];
  if (indexNum != nil) {
    __strong typeof(_manager) manager = _manager;
    [manager changeView:indexNum withLevel:_currentLevel];
  }
}

- (NSDictionary *)getLogSources {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  return [currentProxy logSources];
}

- (NSString *)getLogSourceWithFileName:(NSString *)fileName {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  return [currentProxy logSourceWithFileName:fileName];
}

- (void)copySnapShotToWindow {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  DevToolLogBoxCache *cache = [[DevToolLogBoxCache alloc] init];
  cache.errorMessages = [currentProxy logMessagesWithLevel:_currentLevel];
  cache.logSources = [self getLogSources];
  cache.errNamespace = [currentProxy getErrorNamespace];
  _window.logBoxCache = cache;
}

- (void)dealloc {
  if (_window) {
    if ([_window isShowing]) {
      [self copySnapShotToWindow];
      [_window onLogBoxDestroy];
    } else {
      [_window destroy];
    }
  }
}

@end
