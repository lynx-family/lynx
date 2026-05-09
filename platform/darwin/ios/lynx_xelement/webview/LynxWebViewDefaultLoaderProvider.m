// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxLazyLoad.h>
#import <XElement/LynxWebViewDefaultLoader.h>

@interface LynxWebViewDefaultLoaderProvider : NSObject <LynxWebViewLoaderProvider>

@end

@implementation LynxWebViewDefaultLoaderProvider

LYNX_LOAD_LAZY([LynxWebViewService sharedInstance].providers[@"default"] =
                   [LynxWebViewDefaultLoaderProvider new];)

- (id<LynxWebViewLoader> _Nullable)createLynxWebViewLoaderWithDelegate:
    (id<LynxWebViewLoaderDelegate>)delegate {
  return [[LynxWebViewDefaultLoader alloc] initWithDelegate:delegate];
}

@end
