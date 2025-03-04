// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import "LynxMeasureDelegate.h"

NS_ASSUME_NONNULL_BEGIN
@class LynxLayoutNode;
@class LynxNativeLayoutNode;

struct MeasureResult {
  CGSize size;
  CGFloat baseline;
};
typedef struct MeasureResult MeasureResult;

// @implementation in LynxLayoutNode.mm
@interface MeasureContext : NSObject
@end

// @implementation in LynxLayoutNode.mm
@interface AlignContext : NSObject
@end

// @implementation in LynxLayoutNode.mm
@interface MeasureParam : NSObject
@property(nonatomic, readonly) CGFloat width;
@property(nonatomic, readonly) LynxMeasureMode widthMode;
@property(nonatomic, readonly) CGFloat height;
@property(nonatomic, readonly) LynxMeasureMode heightMode;

- (id)initWithWidth:(CGFloat)width
          WdithMode:(LynxMeasureMode)widthMode
             Height:(CGFloat)height
         HeightMode:(LynxMeasureMode)heightMode
    __attribute__((deprecated("Use -[initWithWidth:WidthMode:Height:HeightMode] instead.")));

- (id)initWithWidth:(CGFloat)width
          WidthMode:(LynxMeasureMode)widthMode
             Height:(CGFloat)height
         HeightMode:(LynxMeasureMode)heightMode;
@end

// @implementation in LynxLayoutNode.mm
@interface AlignParam : NSObject
@property(nonatomic, readonly) CGFloat leftOffset;
@property(nonatomic, readonly) CGFloat topOffset;

- (void)SetAlignOffsetWithLeft:(CGFloat)leftOffset Top:(CGFloat)topOffset;
@end

@protocol LynxCustomMeasureDelegate <NSObject>
- (MeasureResult)measureWithMeasureParam:(MeasureParam *)param
                          MeasureContext:(nullable MeasureContext *)context;
- (void)alignWithAlignParam:(AlignParam *)param AlignContext:(AlignContext *)context;
@end

NS_ASSUME_NONNULL_END
