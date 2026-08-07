// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

NS_ASSUME_NONNULL_BEGIN

@protocol LynxRuntimeLifecycleListener <NSObject>

/**
 * on runtime detached
 */
- (void)onRuntimeDetach;

/**
 * Callback when napi environment prepared.
 * @param env Napi env in Lynx.*
 * @param runtimeType JS runtime type in Lynx.
 */
@optional
- (void)onRuntimeAttach:(void* _Nonnull)env runtimeType:(const char* _Nonnull)runtimeType;
- (void)onRuntimeAttach:(void* _Nonnull)env;

@end

NS_ASSUME_NONNULL_END
