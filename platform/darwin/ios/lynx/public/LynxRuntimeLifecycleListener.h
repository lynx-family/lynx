// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

NS_ASSUME_NONNULL_BEGIN

@protocol LynxRuntimeLifecycleListener <NSObject>
/**
 * Callback when napi environment prepared.
 * @param env Napi env in Lynx.*
 */

- (void)onRuntimeAttach:(void* _Nonnull)env;

/**
 * on runtime detached
 */
- (void)onRuntimeDetach;
@end

NS_ASSUME_NONNULL_END
