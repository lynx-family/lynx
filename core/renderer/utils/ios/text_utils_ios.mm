// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/ios/text_utils_ios.h"
#import "LynxTemplateData+Converter.h"
#import "LynxTextUtils.h"
#import "LynxUnitUtils.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/value_utils.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

// The input info contains fontSize and fontFamily, both of which are string types. The unit of
// fontSize can be px or rpx.
std::unique_ptr<pub::Value> TextUtilsDarwinHelper::GetTextInfo(const std::string& string,
                                                               const pub::Value& info) {
  lepus::Value result = lepus::Value::CreateObject();
  float width = 0.f;
  if (!string.empty() && info.IsMap()) {
    const std::string font_size = info.GetValueForKey(kFontSize)->str();
    static CGFloat kLynxDefaultFontSize = 14;
    CGFloat fontSize = kLynxDefaultFontSize;
    if (!font_size.empty()) {
      fontSize = [LynxUnitUtils toPtFromUnitValue:[NSString stringWithUTF8String:font_size.c_str()]
                                    withDefaultPt:kLynxDefaultFontSize];
    }
    NSString* fontFamily = nil;
    const std::string font_family = info.GetValueForKey(kFontFamily)->str();
    if (!font_family.empty()) {
      fontFamily = [NSString stringWithUTF8String:font_family.c_str()];
    }
    NSInteger maxLine = info.GetValueForKey(kMaxLine)->Number();
    const std::string max_width = info.GetValueForKey(kMaxWidth)->str();
    CGFloat maxWidth = 0;
    if (!max_width.empty()) {
      maxWidth = [LynxUnitUtils toPtFromUnitValue:[NSString stringWithUTF8String:max_width.c_str()]
                                    withDefaultPt:kLynxDefaultFontSize];
    }
    NSString* text = [NSString stringWithUTF8String:string.c_str()];
    NSDictionary* resultDic = [LynxTextUtils measureText:text
                                                fontSize:fontSize
                                              fontFamily:fontFamily
                                                maxWidth:maxWidth
                                                 maxLine:maxLine];
    return std::make_unique<PubLepusValue>(LynxConvertToLepusValue(resultDic));
  }

  result.SetProperty(BASE_STATIC_STRING(kWidth), lepus::Value(width));
  return std::make_unique<PubLepusValue>(lepus::Value(result));
}

}  // namespace tasm
}  // namespace lynx
