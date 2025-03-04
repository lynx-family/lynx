// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import <Lynx/LynxClassAliasDefines.h>

NS_ASSUME_NONNULL_BEGIN

@protocol FrameCapturerDelegate

@required
- (BOOL)isEnabled;
- (void)takeSnapshot:(VIEW_CLASS*)view;
- (void)onNewSnapshot:(NSString*)data;
- (void)onFrameChanged;

@end

@interface LynxDevToolFrameCapturer : NSObject

- (void)attachView:(VIEW_CLASS*)uiView;
- (void)startFrameViewTrace;
- (void)stopFrameViewTrace;
- (void)screenshot;
- (void)onTakeSnapShot:(NSString*)snapshot;

@property(nonatomic, weak, readwrite, nullable) VIEW_CLASS* uiView;
@property(nonatomic, readwrite, nullable) NSString* snapshotCache;
@property(nonatomic, readwrite) uint64_t snapshotInterval;
@property(nonatomic, readwrite) uint64_t lastScreenshotTime;
@property(nonatomic, readwrite, nullable) CFRunLoopObserverRef observer;
@property(atomic, readwrite) BOOL hasSnapshotTask;

@property(nonatomic, weak, readwrite, nullable) id<FrameCapturerDelegate> delegate;

@end

NS_ASSUME_NONNULL_END
