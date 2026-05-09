// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
#import <WebKit/WebKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxWebViewLoaderInvocationCallback)(BOOL success, NSInteger customErrorCode,
                                                    NSString *_Nullable errorMsg);

@protocol LynxWebViewLoaderDelegate <NSObject>

- (NSString *)nameOfScriptMessageHandler;

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation;

- (void)webView:(WKWebView *)webView
    didFailNavigation:(WKNavigation *)navigation
            withError:(NSError *)error;

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message;

@end

@protocol LynxWebViewLoader <NSObject>
@required

- (instancetype)initWithDelegate:(id<LynxWebViewLoaderDelegate>)delegate;

- (UIView *)getContainerView;

- (WKWebView *)getWebView;

- (void)setParams:(NSDictionary *)params;

- (void)load:(NSString *)urlStr;

- (void)loadHtmlString:(NSString *)htmlString;

- (void)reload;

- (void)evaluateJavaScript:(NSString *)javaScriptString
         completionHandler:
             (void (^_Nullable)(_Nullable id, NSError *_Nullable error))completionHandler;

@end

@protocol LynxWebViewLoaderProvider <NSObject>

- (id<LynxWebViewLoader> _Nullable)createLynxWebViewLoaderWithDelegate:
    (id<LynxWebViewLoaderDelegate>)delegate;

@end

@interface LynxWebViewService : NSObject

@property(nonatomic, strong)
    NSMutableDictionary<NSString *, id<LynxWebViewLoaderProvider>> *_Nullable providers;

+ (instancetype)sharedInstance;

- (id<LynxWebViewLoader> _Nullable)createLynxWebViewLoaderWithKey:(nonnull NSString *)key
                                                         delegate:(id<LynxWebViewLoaderDelegate>)
                                                                      delegate;

@end

@interface LynxUIWebView : LynxUI

@end

NS_ASSUME_NONNULL_END
