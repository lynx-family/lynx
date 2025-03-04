// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_DECODER_H_
#define CORE_RENDERER_CSS_CSS_DECODER_H_

#include <string>

#include "core/renderer/css/auto_gen_css_decoder.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {

class CSSDecoder : public AutoGenCSSDecoder {
 public:
  static std::string CSSValueToString(const lynx::tasm::CSSPropertyID id,
                                      const lynx::tasm::CSSValue& value);
  static std::string CSSValueEnumToString(const lynx::tasm::CSSPropertyID id,
                                          const lynx::tasm::CSSValue& value);

  static std::string CSSValueNumberToString(const lynx::tasm::CSSPropertyID id,
                                            const lynx::tasm::CSSValue& value);

  static std::string CSSValueArrayToString(const lynx::tasm::CSSPropertyID id,
                                           const lynx::tasm::CSSValue& value);

  static std::string ToFlexAlignType(lynx::starlight::FlexAlignType type);

  static std::string ToLengthType(lynx::starlight::LengthValueType type);

  static std::string ToOverflowType(lynx::starlight::OverflowType type);

  static std::string ToTimingFunctionType(
      lynx::starlight::TimingFunctionType type);

  static std::string ToAnimationPropertyType(
      lynx::starlight::AnimationPropertyType type);

  static std::string ToBorderStyleType(lynx::starlight::BorderStyleType type);

  static std::string ToTransformType(lynx::starlight::TransformType type);

  static std::string ToShadowOption(lynx::starlight::ShadowOption option);

  static std::string ToAnimationDirectionType(
      lynx::starlight::AnimationDirectionType type);

  static std::string ToAnimationFillModeType(
      lynx::starlight::AnimationFillModeType type);

  static std::string ToAnimationPlayStateType(
      lynx::starlight::AnimationPlayStateType type);

  static std::string ToRgbaFromColorValue(const std::string& color_value);

  static std::string ToRgbaFromRgbaValue(const std::string& r,
                                         const std::string& g,
                                         const std::string& b,
                                         const std::string& a);

  static std::string ToPxValue(double px_value);

  static std::string ToJustifyType(lynx::starlight::JustifyType type);
  static std::string ToGridAutoFlowType(lynx::starlight::GridAutoFlowType type);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_DECODER_H_
