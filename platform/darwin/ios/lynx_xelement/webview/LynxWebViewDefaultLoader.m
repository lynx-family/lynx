// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxWebViewDefaultLoader.h>

@implementation LynxWebViewDefaultLoader

- (instancetype)initWithDelegate:(id<LynxWebViewLoaderDelegate>)delegate {
  if (self = [super init]) {
    self.delegate = delegate;
  }
  return self;
}

- (UIView *)getContainerView {
  return [self getWebView];
}

- (WKWebView *)getWebView {
  if (!self.webview) {
    WKWebViewConfiguration *configuration = [[WKWebViewConfiguration alloc] init];
    [configuration.userContentController
        addScriptMessageHandler:self
                           name:self.delegate.nameOfScriptMessageHandler];
    self.webview = [[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration];
    self.webview.scrollView.bounces = NO;
    self.webview.scrollView.showsVerticalScrollIndicator = NO;
    self.webview.scrollView.showsHorizontalScrollIndicator = NO;
    self.webview.navigationDelegate = self;
  }
  return self.webview;
}

- (void)setParams:(NSDictionary *)params {
}

- (void)load:(NSString *)urlStr {
  [self.webview loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlStr]]];
}

- (void)loadHtmlString:(NSString *)htmlString {
  [self.webview loadHTMLString:htmlString baseURL:nil];
}

- (void)reload {
  [self.webview reload];
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
  [self.delegate webView:webView didFinishNavigation:navigation];
}

- (void)evaluateJavaScript:(NSString *)javaScriptString
         completionHandler:
             (void (^_Nullable)(_Nullable id, NSError *_Nullable error))completionHandler {
  [self.webview evaluateJavaScript:javaScriptString completionHandler:completionHandler];
}

- (void)webView:(WKWebView *)webView
    didFailNavigation:(WKNavigation *)navigation
            withError:(NSError *)error {
  [self.delegate webView:webView didFailNavigation:navigation withError:error];
}

- (void)webView:(WKWebView *)webView
    didFailProvisionalNavigation:(WKNavigation *)navigation
                       withError:(NSError *)error {
  [self.delegate webView:webView didFailNavigation:navigation withError:error];
}

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
  if ([message.name isEqualToString:self.delegate.nameOfScriptMessageHandler]) {
    [self.delegate userContentController:userContentController didReceiveScriptMessage:message];
  }
}

@end
