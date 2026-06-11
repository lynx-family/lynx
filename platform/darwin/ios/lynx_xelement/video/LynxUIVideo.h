// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <XElement/LynxVideoDefines.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIVideo : LynxUI <LynxVideoPlayableDelegate>

@property(nonatomic, strong, readonly) UIView<LynxVideoPlayable> *videoView;

- (UIView<LynxVideoPlayable> *)createVideoView;

- (void)play:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
- (void)pause:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
- (void)stop:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;
- (void)seek:(NSDictionary *)params withResult:(LynxUIMethodCallbackBlock)callback;

@end

NS_ASSUME_NONNULL_END
