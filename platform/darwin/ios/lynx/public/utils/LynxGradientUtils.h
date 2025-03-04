// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxCSSType.h"
#import "LynxClassAliasDefines.h"

NS_ASSUME_NONNULL_BEGIN
#pragma once
struct LynxLengthContext {
  float screen_width;
  float layouts_unit_per_px;
  float physical_pixels_per_layout_unit;
  float root_node_font_size;
  float cur_node_font_size;
  float font_scale;
  float viewport_width;
  float viewport_height;
  bool font_scale_sp_only;
};

@interface LynxGradientUtils : NSObject

+ (CGPoint)getRadialRadiusWithShape:(LynxRadialGradientShapeType)shape
                          shapeSize:(LynxRadialGradientSizeType)shapeSize
                            centerX:(CGFloat)cx
                            centerY:(CGFloat)cy
                              sizeX:(CGFloat)sx
                              sizeY:(CGFloat)sy;

+ (NSArray* _Nullable)getGradientArrayFromString:(NSString* _Nonnull)gradientDef
                               withLengthContext:(struct LynxLengthContext)context;

@end

NS_ASSUME_NONNULL_END
