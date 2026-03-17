// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGradientUtils.h>

#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"

@implementation LynxGradientUtils

+ (CGPoint)getRadialRadiusWithShape:(LynxRadialGradientShapeType)shape
                          shapeSize:(LynxRadialGradientSizeType)shapeSize
                            centerX:(CGFloat)cx
                            centerY:(CGFloat)cy
                              sizeX:(CGFloat)sx
                              sizeY:(CGFloat)sy {
  CGPoint ret;
  auto radius = lynx::tasm::GetRadialGradientRadius(
      static_cast<lynx::starlight::RadialGradientShapeType>(shape),
      static_cast<lynx::starlight::RadialGradientSizeType>(shapeSize), cx, cy, sx, sy);
  ret.x = radius.first;
  ret.y = radius.second;
  return ret;
}

static void AssembleArray(NSMutableArray* array, const lynx::lepus::Value& value) {
  if (value.IsNumber()) {
    [array addObject:[NSNumber numberWithDouble:value.Number()]];
  } else if (value.IsArray()) {
    NSMutableArray* ocArray = [[NSMutableArray alloc] init];
    const auto& valArray = value.Array();
    for (size_t i = 0; i < valArray->size(); i++) {
      AssembleArray(ocArray, valArray->get(i));
    }
    [array addObject:ocArray];
  }
}

+ (NSArray* _Nullable)getGradientArrayFromString:(NSString* _Nonnull)gradientDef
                               withLengthContext:(struct LynxLengthContext)context {
  auto gradientData = lynx::starlight::CSSStyleUtils::GetGradientArrayFromString(
      [gradientDef cStringUsingEncoding:NSUTF8StringEncoding],
      [gradientDef lengthOfBytesUsingEncoding:NSUTF8StringEncoding],
      lynx::tasm::CssMeasureContext(context.screen_width, context.layouts_unit_per_px,
                                    context.physical_pixels_per_layout_unit,
                                    context.root_node_font_size, context.cur_node_font_size,
                                    lynx::starlight::LayoutUnit(context.viewport_width),
                                    lynx::starlight::LayoutUnit(context.viewport_height)),
      lynx::tasm::CSSParserConfigs());

  if (!gradientData.IsArray()) {
    return nil;
  }
  const auto& gradientArray = gradientData.Array();
  NSMutableArray* result = [NSMutableArray new];

  for (size_t i = 0; i < gradientArray->size(); i++) {
    AssembleArray(result, gradientArray->get(i));
  }
  return result;
}

+ (void)calculateGradientLineWithAngle:(CGFloat)angleDegrees
                                 width:(CGFloat)width
                                height:(CGFloat)height
                            startPoint:(CGPoint*)startPoint
                              endPoint:(CGPoint*)endPoint {
  // Convert CSS angle to standard math angle
  // CSS: 0deg = to top, 90deg = to right
  // Math: 0 = right, 90 = top (counter-clockwise from right)
  // So CSS 0deg (to top) = Math 90deg, CSS 90deg (to right) = Math 0deg
  double angleRad = (angleDegrees - 90.0) * M_PI / 180.0;

  CGFloat centerX = width / 2.0;
  CGFloat centerY = height / 2.0;

  double sinVal = sin(angleRad);
  double cosVal = cos(angleRad);

  // Handle edge cases where cos or sin is 0 (horizontal/vertical gradients)
  double dx = (cosVal == 0) ? INFINITY : fabs(width / 2.0 / cosVal);
  double dy = (sinVal == 0) ? INFINITY : fabs(height / 2.0 / sinVal);

  double distance;
  if (isinf(dx) || isnan(dx)) {
    distance = dy;
  } else if (isinf(dy) || isnan(dy)) {
    distance = dx;
  } else {
    distance = MIN(dx, dy);
  }

  // Fallback if both are invalid
  if (isinf(distance) || isnan(distance)) {
    distance = MAX(width, height);
  }

  startPoint->x = centerX - distance * cosVal;
  startPoint->y = centerY - distance * sinVal;
  endPoint->x = centerX + distance * cosVal;
  endPoint->y = centerY + distance * sinVal;
}

@end
