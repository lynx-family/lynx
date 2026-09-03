// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <XElement/LynxWebViewDefaultLoader.h>
#import <objc/runtime.h>

@interface LynxWebViewLoaderDelegateStub : NSObject <LynxWebViewLoaderDelegate>
@end

@implementation LynxWebViewLoaderDelegateStub

- (NSString *)nameOfScriptMessageHandler {
  return @"keyboardAccessoryViewTest";
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
}

- (void)webView:(WKWebView *)webView
    didFailNavigation:(WKNavigation *)navigation
            withError:(NSError *)error {
}

- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
}

@end

@interface LynxWebViewDefaultLoaderUnitTest : XCTestCase
@end

@implementation LynxWebViewDefaultLoaderUnitTest

- (void)testHideKeyboardAccessoryView {
  LynxWebViewLoaderDelegateStub *delegate = [LynxWebViewLoaderDelegateStub new];
  id<LynxWebViewLoader> loader = [[LynxWebViewDefaultLoader alloc] initWithDelegate:delegate];
  WKWebView *webView = [loader getWebView];

  XCTAssertFalse([[webView valueForKey:@"keyboardAccessoryViewHidden"] boolValue]);
  XCTAssertTrue(class_getMethodImplementation(webView.class, @selector(inputAccessoryView)) !=
                class_getMethodImplementation(WKWebView.class, @selector(inputAccessoryView)));

  [loader setKeyboardAccessoryViewHidden:YES];

  XCTAssertTrue([[webView valueForKey:@"keyboardAccessoryViewHidden"] boolValue]);
  XCTAssertNil(webView.inputAccessoryView);

  [loader setKeyboardAccessoryViewHidden:NO];

  XCTAssertFalse([[webView valueForKey:@"keyboardAccessoryViewHidden"] boolValue]);
  [webView.configuration.userContentController
      removeScriptMessageHandlerForName:delegate.nameOfScriptMessageHandler];
}

@end
