// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Foundation/Foundation.h>
#import "LynxCSSType.h"
#import "LynxCustomMeasureDelegate.h"

@interface LynxLayoutNodeManager : NSObject

- (instancetype)initWithNativePtr:(void *)nativePtr;

- (LynxFlexDirection)getFlexDirection:(NSInteger)sign;

// Now only supports computed length, length with auto and percentage will be 0
- (CGFloat)getMarginLeft:(NSInteger)sign;
- (CGFloat)getMarginRight:(NSInteger)sign;
- (CGFloat)getMarginTop:(NSInteger)sign;
- (CGFloat)getMarginBottom:(NSInteger)sign;
- (CGFloat)getPaddingLeft:(NSInteger)sign;
- (CGFloat)getPaddingRight:(NSInteger)sign;
- (CGFloat)getPaddingTop:(NSInteger)sign;
- (CGFloat)getPaddingBottom:(NSInteger)sign;
- (CGFloat)getWidth:(NSInteger)sign;
- (CGFloat)getHeight:(NSInteger)sign;
- (CGFloat)getMinWidth:(NSInteger)sign;
// if max-width is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
- (CGFloat)getMaxWidth:(NSInteger)sign;
- (CGFloat)getMinHeight:(NSInteger)sign;
// if max-height is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
- (CGFloat)getMaxHeigh:(NSInteger)sign;

- (void)setMeasureFuncWithSign:(NSInteger)sign LayoutNode:(LynxLayoutNode *)layoutNode;
- (void)markDirtyAndRequestLayout:(NSInteger)sign;
- (void)markDirtyAndForceLayout:(NSInteger)sign;
- (bool)isDirty:(NSInteger)sign;

- (MeasureResult)measureWithSign:(NSInteger)sign
                    MeasureParam:(MeasureParam *)param
                  MeasureContext:(MeasureContext *)context;
- (void)alignWithSign:(NSInteger)sign
           AlignParam:(AlignParam *)param
         AlignContext:(AlignContext *)context;

@end
