// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewBuilder.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, TestBenchReplayError) {
  TestBenchDownloadError = 0,
};

@class TestBenchActionManager;

@interface TestBenchActionCallback : NSObject
- (void)onLynxViewWillBuild:(TestBenchActionManager *)manager builder:(LynxViewBuilder *)builder;
- (void)onLynxViewDidBuild:(LynxView *)lynxView;
- (void)onError:(TestBenchReplayError)code message:(NSString *)message;
@end

NS_ASSUME_NONNULL_END
