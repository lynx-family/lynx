// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TasmDispatcher.h"
#import <Lynx/LynxLog.h>
#import "LynxExplorerSwiftInterop.h"

@interface TasmDispatcher ()

- (void)forwardURL:(NSString *)sourceUrl
            source:(LXRouteSource)source
      presentation:(LXRoutePresentation)presentation;

@end

// This facade intentionally implements its own deprecated methods, so silence
// the deprecation warning for the implementation only.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"

@implementation TasmDispatcher

static TasmDispatcher *_instance = nil;

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[self alloc] init];
  });
  return _instance;
}

- (void)openTargetUrlSingleTop:(NSString *)sourceUrl {
  [self forwardURL:sourceUrl
            source:LXRouteSourceDebugBridge
      presentation:LXRoutePresentationReplaceTop];
}

- (void)openTargetUrl:(NSString *)sourceUrl {
  [self forwardURL:sourceUrl source:LXRouteSourceNativeModule presentation:LXRoutePresentationPush];
}

- (void)forwardURL:(NSString *)sourceUrl
            source:(LXRouteSource)source
      presentation:(LXRoutePresentation)presentation {
  LXRouteCoordinator *coordinator = [LXRouteCoordinator currentBridge];
  if (coordinator == nil) {
    LLogWarn(@"Lynx Explorer route coordinator has not been installed.");
    return;
  }
  [coordinator openURL:sourceUrl
      requestedContainer:LXRequestedContainerAutomatic
                  source:source
            presentation:presentation
        animatedOverride:nil
                callback:^(id result) {
                  NSDictionary *payload =
                      [result isKindOfClass:[NSDictionary class]] ? result : nil;
                  if (payload != nil && ![payload[@"success"] boolValue]) {
                    LLogWarn(@"Lynx Explorer route failed: %@", payload[@"message"]);
                  }
                }];
}

@end

#pragma clang diagnostic pop
