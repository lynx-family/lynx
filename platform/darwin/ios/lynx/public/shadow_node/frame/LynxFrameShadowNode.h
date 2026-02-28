// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxCustomMeasureShadowNode.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxFrameShadowNode : LynxCustomMeasureShadowNode

/// The intrinsic content size received from the nested LynxView
@property(nonatomic, assign) CGSize intrinsicContentSize;

/// Updates the intrinsic content size and triggers re-layout
/// @param size The new intrinsic content size from the nested LynxView
- (void)updateIntrinsicContentSize:(CGSize)size;

@end

NS_ASSUME_NONNULL_END
