// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGradientUtils.h"

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

@end
