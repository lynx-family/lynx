// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@class LynxLayoutNode;

typedef NS_ENUM(NSInteger, LynxMeasureMode) {
  LynxMeasureModeIndefinite = 0,
  LynxMeasureModeDefinite = 1,
  LynxMeasureModeAtMost = 2
};

NS_ASSUME_NONNULL_BEGIN

@protocol LynxMeasureDelegate <NSObject>

- (CGSize)measureNode:(LynxLayoutNode*)node
            withWidth:(CGFloat)width
            widthMode:(LynxMeasureMode)widthMode
               height:(CGFloat)height
           heightMode:(LynxMeasureMode)heightMode;
@end

NS_ASSUME_NONNULL_END
