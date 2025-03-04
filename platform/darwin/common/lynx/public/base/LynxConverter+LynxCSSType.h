// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXCONVERTER_LYNXCSSTYPE_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXCONVERTER_LYNXCSSTYPE_H_

#import <Foundation/Foundation.h>
#import "LynxConverter.h"

NS_ASSUME_NONNULL_BEGIN

DECLARE_ENUM_CONVERTER(LynxOverflowType)
DECLARE_ENUM_CONVERTER(LynxAnimationDirectionType)
DECLARE_ENUM_CONVERTER(CAMediaTimingFillMode)
DECLARE_ENUM_CONVERTER(LynxAnimationPlayStateType)
DECLARE_ENUM_CONVERTER(LynxVisibilityType)
DECLARE_ENUM_CONVERTER(LynxDirectionType)
DECLARE_ENUM_CONVERTER(LynxFontWeightType)
DECLARE_ENUM_CONVERTER(LynxWordBreakType)
DECLARE_ENUM_CONVERTER(LynxFontStyleType)
DECLARE_ENUM_CONVERTER(LynxTextAlignType)
DECLARE_ENUM_CONVERTER(LynxWhiteSpaceType)
DECLARE_ENUM_CONVERTER(LynxTextOverflowType)
DECLARE_ENUM_CONVERTER(LynxTextDecorationType)
DECLARE_ENUM_CONVERTER(LynxVerticalAlign)

#undef DECLARE_ENUM_CONVERTER

@interface LynxConverter (CAMediaTimingFunction)
+ (CAMediaTimingFunction*)toCAMediaTimingFunction:(id __nullable)value;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXCONVERTER_LYNXCSSTYPE_H_
