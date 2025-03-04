// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxShadowNode.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxCustomMeasureShadowNode : LynxShadowNode

@property(nonatomic, assign) BOOL hasCustomLayout;

// Call this method to measure child node
- (MeasureResult)measureNativeLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)context;

// Call this method to align child node
- (void)alignNativeLayoutNode:(nonnull AlignParam *)param
                 alignContext:(nonnull AlignContext *)context;

// Subclass should overwrite this implement
- (MeasureResult)customMeasureLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)context;

// Subclass should overwrite this implement
- (void)customAlignLayoutNode:(nonnull AlignParam *)param
                 alignContext:(nonnull AlignContext *)context;

- (CGFloat)toPtWithUnitValue:(NSString *)unitValue fontSize:(CGFloat)fontSize;

@end

NS_ASSUME_NONNULL_END
