// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRuntimeLifecycleListener.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxRuntimeFullLifecycleListener <LynxRuntimeLifecycleListener>
/**
 Callback when runtime is ready.
 @param vsyncObserver VSyncObserver, share raf callbacks queue with Lynx.
 */
- (void)onRuntimeCreate:(void* _Nonnull)vsyncObserver;
/**
 Callback when lynx runtime is inited.
 @param runtimeId Current runtime id in Lynx.
 */
- (void)onRuntimeInit:(long)runtimeId;
/**
 Callback when app onshow
 */
- (void)onAppEnterForeground;
/**
 Callback when app onHide.
 */
- (void)onAppEnterBackground;
@end

NS_ASSUME_NONNULL_END
