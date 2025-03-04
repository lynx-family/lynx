// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxShadowNode.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"

NS_ASSUME_NONNULL_BEGIN

@interface PaintingContextProxy : NSObject <LynxShadowNodeDelegate>

- (instancetype)initWithPaintingContext:(lynx::tasm::PaintingContextDarwin*)paintingContext;

/**
 * Get layout status
 */
- (BOOL)isLayoutFinish;

/**
 * Update the status of the layout to unfinished
 */
- (void)resetLayoutStatus;

@end

NS_ASSUME_NONNULL_END
