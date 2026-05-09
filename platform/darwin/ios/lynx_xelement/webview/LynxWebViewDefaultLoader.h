// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>
#import <XElement/LynxUIWebView.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxWebViewDefaultLoader
    : NSObject <LynxWebViewLoader, WKScriptMessageHandler, WKNavigationDelegate>
@property(nonatomic, strong) WKWebView *webview;
@property(nonatomic, weak) id<LynxWebViewLoaderDelegate> delegate;
@end

NS_ASSUME_NONNULL_END
