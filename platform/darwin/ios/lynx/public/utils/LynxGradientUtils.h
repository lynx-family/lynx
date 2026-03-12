// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxCSSType.h>
#import <Lynx/LynxClassAliasDefines.h>

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

/**
 * Calculates the start and end points for a linear gradient line using a CSS angle.
 * This method implements the distance-based algorithm for consistent cross-platform behavior.
 *
 * @param angleDegrees The CSS gradient angle in degrees (0deg = to top, 90deg = to right)
 * @param width The width of the gradient box
 * @param height The height of the gradient box
 * @param startPoint Output parameter for the calculated start point (in box coordinates)
 * @param endPoint Output parameter for the calculated end point (in box coordinates)
 */
+ (void)calculateGradientLineWithAngle:(CGFloat)angleDegrees
                                 width:(CGFloat)width
                                height:(CGFloat)height
                            startPoint:(CGPoint*)startPoint
                              endPoint:(CGPoint*)endPoint;

@end

NS_ASSUME_NONNULL_END
