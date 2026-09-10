// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxCustomMeasureShadowNode.h>
#import <Lynx/LynxUI.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIOverlay : LynxUI <UIView *>

// Update custom layout bounds only when the overlay is visible and follows its mode's bounds.
- (void)updateCustomRectIfNeeded;

@end

@interface LynxUIOverlayShadowNode : LynxCustomMeasureShadowNode

@end

NS_ASSUME_NONNULL_END
