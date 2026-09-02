// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropertyDiffMap.h>
#import <Lynx/LynxPropsProcessor.h>
#import <XCTest/XCTest.h>
#import <XElement/LynxUIWebView.h>
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

@interface LynxFirstResponderView : UIView
@property(nonatomic, assign) BOOL didReloadInputViews;
@end

@implementation LynxFirstResponderView

- (BOOL)isFirstResponder {
  return YES;
}

- (void)reloadInputViews {
  self.didReloadInputViews = YES;
}

@end

@interface LynxWebViewDefaultLoaderUnitTest : XCTestCase
@end

@implementation LynxWebViewDefaultLoaderUnitTest

- (void)testWebViewPropertyRegistration {
  LynxUIWebView *ui = [LynxUIWebView new];
  LynxPropertyDiffMap *diffMap = [ui valueForKey:@"diffMap"];
  NSDictionary<NSString *, id> *values = @{
    @"bounces" : @NO,
    @"scroll-bar-enable" : @YES,
    @"params" : @{@"key" : @"value"},
    @"src" : @"about:blank",
    @"html" : @"<input>",
    @"enable-debug" : @YES,
    @"ios-hide-keyboard-accessory-view" : @YES
  };
  for (NSString *key in values) {
    [LynxPropsProcessor updateProp:values[key] withKey:key forUI:ui];
    XCTAssertEqualObjects([diffMap getValueForKey:key], values[key], @"%@", key);
  }
  [LynxPropsProcessor updateProp:@"custom" withKey:@"webview-type" forUI:ui];
  XCTAssertEqualObjects([ui valueForKey:@"loaderType"], @"custom");

  [LynxPropsProcessor updateProp:@YES withKey:@"bounces" forUI:ui];
  NSDictionary<NSString *, id> *resetValues = @{
    @"bounces" : @NO,
    @"scroll-bar-enable" : @NO,
    @"params" : @{},
    @"src" : @"",
    @"html" : @"",
    @"enable-debug" : @NO,
    @"ios-hide-keyboard-accessory-view" : @NO
  };
  for (NSString *key in resetValues) {
    [LynxPropsProcessor updateProp:nil withKey:key forUI:ui];
    XCTAssertEqualObjects([diffMap getValueForKey:key], resetValues[key], @"%@", key);
  }
  [LynxPropsProcessor updateProp:nil withKey:@"webview-type" forUI:ui];
  XCTAssertEqualObjects([ui valueForKey:@"loaderType"], @"");
}

- (void)testHideKeyboardAccessoryView {
  LynxWebViewLoaderDelegateStub *delegate = [LynxWebViewLoaderDelegateStub new];
  id<LynxWebViewLoader> loader = [[LynxWebViewDefaultLoader alloc] initWithDelegate:delegate];
  WKWebView *webView = [loader getWebView];

  XCTAssertFalse([[webView valueForKey:@"keyboardAccessoryViewHidden"] boolValue]);
  XCTAssertTrue(class_getMethodImplementation(webView.class, @selector(inputAccessoryView)) !=
                class_getMethodImplementation(WKWebView.class, @selector(inputAccessoryView)));

  LynxFirstResponderView *firstResponder = [LynxFirstResponderView new];
  [webView addSubview:firstResponder];

  [loader setKeyboardAccessoryViewHidden:YES];

  XCTAssertTrue([[webView valueForKey:@"keyboardAccessoryViewHidden"] boolValue]);
  XCTAssertNil(webView.inputAccessoryView);
  XCTAssertTrue(firstResponder.didReloadInputViews);

  firstResponder.didReloadInputViews = NO;
  [loader setKeyboardAccessoryViewHidden:YES];

  XCTAssertFalse(firstResponder.didReloadInputViews);

  [loader setKeyboardAccessoryViewHidden:NO];

  XCTAssertFalse([[webView valueForKey:@"keyboardAccessoryViewHidden"] boolValue]);
  XCTAssertTrue(firstResponder.didReloadInputViews);
  [webView.configuration.userContentController
      removeScriptMessageHandlerForName:delegate.nameOfScriptMessageHandler];
}

@end
