// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLogBox.h"
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <LynxDevtool/LynxDevToolDownloader.h>
#import <LynxDevtool/LynxDevToolToast.h>
#import <LynxDevtool/LynxDevtoolEnv.h>
#import <TargetConditionals.h>
#import <WebKit/WebKit.h>

static NSString *const kLogBoxEventReportFeedback = @"lynxsdk_redbox_feedback";

typedef NS_ENUM(NSInteger, LynxLogBoxErrorType) {
  LogBoxErrorTypeUnknown = -1,
  LogBoxErrorTypeJS = 1,
  LogBoxErrorTypeLepus = 2,
  LogBoxErrorTypeLepusNG = 3,
  LogBoxErrorTypeNative = 4,
};

#if OS_OSX
#define LynxLogBoxBaseWindow NSView
#else
#define LynxLogBoxBaseWindow UIWindow
#endif
@class LynxLogBoxWindow;

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
@interface LynxLogBoxCache : NSObject

@property(nonatomic, readwrite) NSMutableArray *errorMessages;
@property(nonatomic, readwrite) NSMutableArray *logMessages;
@property(nonatomic, readwrite) NSDictionary *jsSource;

- (instancetype)init;

@end

@implementation LynxLogBoxCache

- (instancetype)init {
  if (self = [super init]) {
    self.errorMessages = nil;
    self.logMessages = nil;
    self.jsSource = nil;
  }
  return self;
}

@end

#pragma mark - LynxLogBoxWindowActionDelegate
@protocol LynxLogBoxWindowActionDelegate <NSObject>
@optional
// Only needed in LynxLogBox
- (void)clearCurrentLogs;
@optional
- (void)changeView:(NSDictionary *)params;
@optional
- (void)dismiss;
- (void)reloadFromLogBoxWindow:(LynxLogBoxWindow *)logBoxWindow;
- (NSDictionary *)getAllJsSource;
// Get instance id of LynxContext
- (int32_t)getInstanceId;
@end

#pragma mark - LynxLogBoxWindow
@interface LynxLogBoxWindow : LynxLogBoxBaseWindow <WKScriptMessageHandler, WKNavigationDelegate>
@property(nonatomic, weak) id<LynxLogBoxWindowActionDelegate> actionDelegate;
@property(nonatomic, copy) void (^loadingFinishCallback)();
@property(nonatomic, readwrite) LynxLogBoxCache *logBoxCache;

- (BOOL)isShowing;
- (void)onLogBoxDestroy;
- (void)destroy;

@end

@implementation LynxLogBoxWindow {
  WKWebView *_stackTraceWebView;
  NSDictionary *_jsAPI;
  NSString *_templateUrl;
  BOOL _isLogBoxDestroyed;
#if OS_OSX
  // click background to dismiss log box
  NSView *_clickGestureView;
#else
  __weak UIWindow *_previous_keywindow;
#endif
}

- (instancetype)initWithFrame:(CGRect)frame
                          URL:(NSString *)templateUrl
             supportYellowBox:(BOOL)support {
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
        @"getCoreJs" : ^(NSDictionary *params, NSNumber *callbackId) {
            NSDictionary *msg = [NSDictionary
                dictionaryWithObjectsAndKeys:callbackId, @"callbackId",
                                             [weakSelf getJsSource:@"core.js"], @"data", nil];
            [weakSelf sendJsResult:msg];
        },
        @"getTemplateJs": ^(NSDictionary *params, NSNumber *callbackId) {
          NSString *key = params ? params[@"name"] : nil;
          NSDictionary *msg = [NSDictionary
              dictionaryWithObjectsAndKeys:callbackId, @"callbackId",
                                           [weakSelf getJsSource:key], @"data", nil];
           [weakSelf sendJsResult:msg];
        },
        @"deleteLynxview": ^(NSDictionary *params, NSNumber *callbackId) {
            [weakSelf clearCurrentLogs];
        },
        @"changeView": ^(NSDictionary *params, NSNumber *callbackId) {
          [weakSelf changeView:params];
        },
        @"reload": ^(NSDictionary *params, NSNumber *callbackId) {
          [weakSelf reload];
        },
        @"dismiss": ^(NSDictionary *params, NSNumber *callbackId) {
          [weakSelf dismiss];
        },
        @"download": ^(NSDictionary *params, NSNumber *callbackId) {
            NSString *url = params ? params[@"url"] : nil;
            [weakSelf download:url withCallbackId:callbackId];
        },
        @"toast": ^(NSDictionary *params, NSNumber *callbackId) {
            NSString *message = params ? params[@"message"] : nil;
            [LynxDevToolToast showToast:message];
        },
        @"reportEvent": ^(NSDictionary *params, NSNumber *callbackId) {
            [weakSelf handleReporting:params];
        },
        @"queryResource": ^(NSDictionary *params, NSNumber *callbackId) {
            NSString *name = params ? params[@"name"] : nil;
            [weakSelf getResource:name withCallbackId:callbackId];
        },
    };
    _templateUrl = templateUrl;
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
      NSURL *url = [self getLogBoxPageUrlWithSupportYellowBox:support];
      if (url) {
          [_stackTraceWebView loadFileURL:url allowingReadAccessToURL:url];
          [self resizeWindow];
      } else {
          LLogError(@"get log box page url failed");
      }
#if OS_OSX
/**
 * When lynx view destroyed, LynxLogBoxWindow might still exist (#4780).
 * At the moment we reload lynx view, we need remove previous window (NSView).
 */
    for (NSView *window in [NSApplication sharedApplication]
             .mainWindow.contentView.subviews.reverseObjectEnumerator) {
      if ([window isKindOfClass:[LynxLogBoxWindow class]]) {
        if (((LynxLogBoxWindow *)window)->_isLogBoxDestroyed) {
          [(LynxLogBoxWindow *)window destroy];
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

- (NSURL *)getLogBoxPageUrlWithSupportYellowBox:(BOOL)supportYellowBox {
  NSURL *url;
  NSURL *debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                  withExtension:@"bundle"];
  if (debugBundleUrl) {
    NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
    url = [bundle URLForResource:@"logbox/index" withExtension:@".html"];
    if (!url) {
      return nil;
    }
    NSURLComponents *components = [[NSURLComponents alloc] initWithURL:url
                                               resolvingAgainstBaseURL:NO];
    NSMutableArray<NSURLQueryItem *> *queryItems = [[NSMutableArray alloc] init];
    [queryItems addObject:[[NSURLQueryItem alloc] initWithName:@"url" value:_templateUrl]];
    [queryItems addObject:[[NSURLQueryItem alloc] initWithName:@"downloadapi" value:@"true"]];
    if (supportYellowBox) {
      [queryItems addObject:[[NSURLQueryItem alloc] initWithName:@"supportyellowbox"
                                                           value:@"true"]];
    }
    [components setQueryItems:queryItems];
    url = [components URL];
  }
  return url;
}

- (NSString *)getJsSource:(NSString *)key {
  if (!key) {
    return nil;
  }
  NSDictionary *jsSource = nil;
  if (!_isLogBoxDestroyed) {
    jsSource = [_actionDelegate getAllJsSource];
  } else {
    jsSource = _logBoxCache.jsSource;
  }
  return jsSource != nil ? jsSource[key] : nil;
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

- (void)showLogMessage:(NSDictionary *)message {
  [self sendJsEvent:@{@"event" : @"receiveNewLog", @"data" : [self normalizeMessage:message]}];
}

- (void)showErrorMessage:(NSString *)message {
  [self sendJsEvent:@{@"event" : @"receiveNewError", @"data" : message}];
}

- (void)showWarnMessage:(NSString *)message {
  NSString *validMessage = [message substringToIndex:MIN((NSUInteger)10000, message.length)];
  [self sendJsEvent:@{@"event" : @"receiveNewWarning", @"data" : validMessage}];
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
    _previous_keywindow = [LynxUIKitAPIAdapter getKeyWindow];
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

- (void)reload {
  if (_actionDelegate) {
    [_actionDelegate reloadFromLogBoxWindow:self];
  }
}

- (void)download:(NSString *)url withCallbackId:(NSNumber *)callbackId {
  if (url) {
    [LynxDevToolDownloader
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

  NSDictionary *jsSource = nil;
  if (!_isLogBoxDestroyed) {
    jsSource = [_actionDelegate getAllJsSource];
  } else {
    jsSource = _logBoxCache.jsSource;
  }

  for (NSString *key in jsSource) {
    if ([key containsString:name]) {
      NSString *value = jsSource[key];
      NSDictionary *data = [NSDictionary
          dictionaryWithObjectsAndKeys:callbackId, @"callbackId", value, @"data", nil];
      [self sendJsResult:data];
      return;
    }
  }
  [self sendJsResult:emptyData];
}

- (void)reportEvent:(NSString *)eventName
    withPropsBuilder:(NSDictionary<NSString *, NSObject *> * (^)(void))propsBuilder {
  int32_t instance_id = [_actionDelegate getInstanceId];
  [LynxEventReporter onEvent:eventName instanceId:instance_id propsBuilder:propsBuilder];
}

/**
 * Handle events that need to be reported
 * @param data event data containing the event name and props
 */
- (void)handleReporting:(NSDictionary *)data {
  if (!data) {
    return;
  }
  NSString *eventName = data[@"eventName"];
  NSDictionary *eventProps = data[@"eventProps"];
  if (!eventName || eventName.length == 0 || !eventProps) {
    LLogInfo(@"receive invalid event");
    return;
  }
  if ([eventName isEqual:@"feedback"]) {
    [self reportFeedback:eventProps];
  }
}

/**
 * Handle feedback reporting
 * @param eventProps
 * {
 *     "error": object,
 *     "sourceMapTypes": string,
 *     "isParseStackSuccess": boolean,
 *     "isSatisfied": boolean
 * }
 */
- (void)reportFeedback:(NSDictionary *)eventProps {
  NSDictionary *errorInfo = eventProps[@"error"];
  if (!errorInfo) {
    return;
  }

  [self reportEvent:kLogBoxEventReportFeedback
      withPropsBuilder:^NSDictionary<NSString *, NSObject *> * {
        NSMutableDictionary *props = [NSMutableDictionary dictionary];
        NSNumber *errorType =
            [LynxLogBoxWindow getObject:@"errorType"
                               fromDict:errorInfo
                            withDefault:[NSNumber numberWithInteger:LogBoxErrorTypeUnknown]];
        // put data of event props
        [props setObject:[LynxLogBoxWindow getObject:@"isSatisfied"
                                            fromDict:eventProps
                                         withDefault:@NO]
                  forKey:@"is_satisfied"];
        [props setObject:[LynxLogBoxWindow getObject:@"sourceMapTypes"
                                            fromDict:eventProps
                                         withDefault:@""]
                  forKey:@"source_map_types"];
        [props setObject:[LynxLogBoxWindow getObject:@"isParseStackSuccess"
                                            fromDict:eventProps
                                         withDefault:@NO]
                  forKey:@"is_stack_parse_success"];
        // put data of error info
        [props setObject:[LynxLogBoxWindow getObject:@"message" fromDict:errorInfo withDefault:@""]
                  forKey:@"error_message"];
        [props setObject:[LynxLogBoxWindow getObject:@"stack" fromDict:errorInfo withDefault:@""]
                  forKey:@"original_stack"];
        switch ([errorType integerValue]) {
          case LogBoxErrorTypeNative:
            [props setObject:[LynxLogBoxWindow getObject:@"code" fromDict:errorInfo withDefault:@-1]
                      forKey:@"code"];
            [props setObject:[LynxLogBoxWindow getObject:@"fixSuggestion"
                                                fromDict:errorInfo
                                             withDefault:@""]
                      forKey:@"fix_suggestion"];
            [props setObject:[LynxLogBoxWindow getObject:@"context"
                                                fromDict:errorInfo
                                             withDefault:@""]
                      forKey:@"context"];
            [props setObject:[LynxLogBoxWindow getObject:@"nativeStack"
                                                fromDict:errorInfo
                                             withDefault:@""]
                      forKey:@"native_stack"];
            break;
          case LogBoxErrorTypeJS:
          case LogBoxErrorTypeLepus:
          case LogBoxErrorTypeLepusNG:
            [props setObject:[LynxLogBoxWindow getObject:@"debugUrl"
                                                fromDict:errorInfo
                                             withDefault:@""]
                      forKey:@"debug_info_url"];
            break;
          default:
            break;
        }
        return props;
      }];
}

+ (nonnull id)getObject:(NSString *)key
               fromDict:(NSDictionary *)dict
            withDefault:(nonnull id)defaultValue {
  if (!dict || !key || key.length == 0) {
    return defaultValue;
  }
  id value = [dict objectForKey:key];
  return value != nil ? value : defaultValue;
}

- (BOOL)isShowing {
  return !self.isHidden;
}

- (void)onLogBoxDestroy {
  _isLogBoxDestroyed = true;
}

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
  if (!_isLogBoxDestroyed) {
    [self loadMappingsWasm];
    if (self.loadingFinishCallback) {
      self.loadingFinishCallback();
    }
  } else {
    [self.logBoxCache.logMessages
        enumerateObjectsUsingBlock:^(NSDictionary *_Nonnull message, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          [self showLogMessage:message];
        }];
    [self.logBoxCache.errorMessages
        enumerateObjectsUsingBlock:^(NSString *_Nonnull message, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          [self showErrorMessage:message];
        }];
  }
}

- (void)webView:(WKWebView *)webView
    didFailProvisionalNavigation:(WKNavigation *)navigation
                       withError:(NSError *)error {
  if (error && !_stackTraceWebView.isLoading) {
    LLogInfo(@"Load log box page failed: %@", [error localizedDescription]);
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
  NSURL *debugBundleUrl = [[NSBundle mainBundle] URLForResource:@"LynxDebugResources"
                                                  withExtension:@"bundle"];
  if (!debugBundleUrl) {
    LLogWarn(@"Failed to load mappings.wasm: resource bundle not exists");
    return;
  }
  NSBundle *bundle = [NSBundle bundleWithURL:debugBundleUrl];
  NSString *path = [bundle pathForResource:@"logbox/mappings" ofType:@"wasm"];
  if (!path) {
    LLogWarn(@"Failed to load mappings.wasm: file not exists");
    return;
  }
  NSData *fileData = [NSData dataWithContentsOfFile:path];
  if (!fileData) {
    LLogWarn(@"Failed to load mappings.wasm: can not load file content data");
    return;
  }
  NSString *base64Data = [fileData base64EncodedStringWithOptions:0];
  NSDictionary *event =
      @{@"event" : @"loadFile", @"data" : @{@"type" : @"mappings.wasm", @"data" : base64Data}};
  [self sendJsEvent:event];
}

@end

#pragma mark - LynxLogBox
@interface LynxLogBox () <LynxLogBoxWindowActionDelegate>
@end

@implementation LynxLogBox {
  __weak LynxLogBoxManager *_manager;
  LynxLogBoxWindow *_window;
  __weak LynxLogBoxProxy *_currentProxy;
  LynxLogBoxLevel _currentLevel;
  NSString *_templateUrl;
  // start from 1, send updateViewInfo to webview when it changes, default for -1.
  NSInteger _index;
  NSInteger _count;
  bool _isLoadingFinished;
  bool _isConsoleOnly;
}

- (instancetype)initWithLogBoxManager:(LynxLogBoxManager *)manager {
  _window = nil;
  _manager = manager;
  _currentProxy = nil;
  _templateUrl = @"";
  _index = -1;
  _count = -1;
  _isLoadingFinished = false;
  _isConsoleOnly = false;
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

- (void)updateTemplateUrl:(NSString *)url {
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
  LLogInfo(@"logbox: logbox updateViewInfoOnMainThread, url: %@", url);
  if (index != _index || count != _count || ![_templateUrl isEqualToString:url]) {
    _index = index;
    _count = count;
    _templateUrl = url != nil ? url : @"";
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
    @"type" : _currentLevel == LynxLogBoxLevelWarning ? @"yellowbox" : @"redbox",
    @"templateUrl" : _templateUrl != nil ? _templateUrl : @""
  }];
}

- (BOOL)onNewLog:(NSString *)message
       withLevel:(LynxLogBoxLevel)level
       withProxy:(LynxLogBoxProxy *)proxy {
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

- (BOOL)onNewConsole:(NSDictionary *)message withProxy:(LynxLogBoxProxy *)proxy isOnly:(BOOL)only {
  if ([NSThread isMainThread]) {
    [self showConsoleMessageOnMainThread:message withProxy:proxy isOnly:only];
  } else {
    __weak __typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf showConsoleMessageOnMainThread:message withProxy:proxy isOnly:only];
    });
  }
  return _isLoadingFinished;
}

- (void)showLogMessageOnMainThread:(NSString *)message
                         withLevel:(LynxLogBoxLevel)level
                         withProxy:(LynxLogBoxProxy *)proxy {
  _currentProxy = proxy;
  _currentLevel = level;
  NSString *url = [proxy templateUrl];
  _templateUrl = url != nil ? url : @"";
  _isConsoleOnly = false;
  if (_isLoadingFinished) {
    [self showLogMessageOnWindow:message];
  }
  [self showOnMainThread];
}

- (void)showLogMessageOnWindow:(NSString *)message {
  if (_currentLevel == LynxLogBoxLevelWarning) {
    [_window showWarnMessage:message];
  } else {
    [_window showErrorMessage:message];
  }
}

- (void)showConsoleMessageOnMainThread:(NSDictionary *)message
                             withProxy:(LynxLogBoxProxy *)proxy
                                isOnly:(BOOL)only {
  _currentProxy = proxy;
  NSString *url = [proxy templateUrl];
  _templateUrl = url != nil ? url : @"";
  _isConsoleOnly = only;
  if (_isLoadingFinished) {
    [_window showLogMessage:message];
  }
  if (_isConsoleOnly) {
    [self showOnMainThread];
  }
}

- (void)showOnMainThread {
  if (!self->_window) {
    CGRect windowFrame;
#if TARGET_OS_OSX
    windowFrame = [NSApplication sharedApplication].mainWindow.frame;
#else
    windowFrame = [UIScreen mainScreen].bounds;
#endif
    self->_window = [[LynxLogBoxWindow alloc] initWithFrame:windowFrame
                                                        URL:_templateUrl
                                           supportYellowBox:YES];
    self->_window.actionDelegate = self;
    __weak __typeof(self) weakSelf = self;
    [self->_window setLoadingFinishCallback:^{
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      if (strongSelf) {
        strongSelf->_isLoadingFinished = true;
        [strongSelf updateViewInfoOnWindow];
        if (!strongSelf->_isConsoleOnly) {
          [[strongSelf getCurrentLogMsgs]
              enumerateObjectsUsingBlock:^(NSString *_Nonnull message, NSUInteger idx,
                                           BOOL *_Nonnull stop) {
                [strongSelf showLogMessageOnWindow:message];
              }];
        }
        [[strongSelf getCurrentConsoleMsgs]
            enumerateObjectsUsingBlock:^(NSDictionary *_Nonnull message, NSUInteger idx,
                                         BOOL *_Nonnull stop) {
              [strongSelf->_window showLogMessage:message];
            }];
      }
    }];
  }
  [self->_window show];
}

- (NSMutableArray *)getCurrentLogMsgs {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  return [currentProxy logMessagesWithLevel:_currentLevel];
}

- (NSMutableArray *)getCurrentConsoleMsgs {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  return [currentProxy consoleMessages];
}

- (BOOL)isShowing {
  return [_window isShowing];
}

- (BOOL)isConsoleOnly {
  return _isConsoleOnly;
}

- (LynxLogBoxLevel)getCurrentLevel {
  return _currentLevel;
}

- (nullable LynxLogBoxProxy *)getCurrentProxy {
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
  _templateUrl = @"";
}

- (void)reloadFromLogBoxWindow:(LynxLogBoxWindow *)logBoxWindow {
  [self->_window dismiss];
  __strong typeof(_manager) manager = _manager;
  [manager reloadFromLogBox:_currentProxy];
}

- (void)changeView:(NSDictionary *)params {
  NSNumber *indexNum = [params objectForKey:@"viewNumber"];
  if (indexNum != nil) {
    __strong typeof(_manager) manager = _manager;
    [manager changeView:indexNum withLevel:_currentLevel];
  }
}

- (NSDictionary *)getAllJsSource {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  return [currentProxy allJsSource];
}

- (int32_t)getInstanceId {
  __strong typeof(_currentProxy) currentProxy = _currentProxy;
  return [currentProxy getInstanceId];
}

- (void)copySnapShotToWindow {
  LynxLogBoxCache *cache = [[LynxLogBoxCache alloc] init];
  cache.errorMessages = [self getCurrentLogMsgs];
  cache.logMessages = [self getCurrentConsoleMsgs];
  cache.jsSource = [self getAllJsSource];
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
