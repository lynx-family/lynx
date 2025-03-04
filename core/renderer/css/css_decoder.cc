// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_decoder.h"

#include <sstream>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_color.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/style/timing_function_data.h"
#include "core/style/transform_raw_data.h"

namespace lynx {
namespace tasm {

using namespace lynx::starlight;  // NOLINT
using namespace lynx::tasm;       // NOLINT

namespace {
// remove
std::string NumberToString(double number) {
  auto ret = std::to_string(number);
  ret = ret.erase(ret.find_last_not_of('0') + 1, std::string::npos);
  if (ret.back() == '.') {
    ret.pop_back();
  }
  return ret;
}

std::string NumberToRGBAColorString(double number) {
  auto color = static_cast<unsigned int>(number);
  std::stringstream stream;
  stream << std::hex << color;
  auto str = stream.str();
  if (str.length() < 3) {
    LOGE("str format is invalid:" << str);
    return str;
  }
  // if the current length is seven or six, which means the hex alpha value is
  // only digit or zero, should insert a zero ahead to make it more easy to
  // understand
  if (str.length() < 8) {
    str.insert(str.begin(), static_cast<size_t>(8 - str.length()), '0');
  }

  auto a = str.substr(0, 2);
  if (a == "ff") {
    str = std::string("#") + str.substr(2, str.length());
  } else {
    str = std::string("#") + str.substr(2, str.length()) + a;
  }
  return str;
}

std::string NumberToLineHeightString(double number) {
  if (number > 10E19) {
    return "normal";
  } else {
    return NumberToString(number);
  }
}

}  // namespace

std::string CSSDecoder::CSSValueToString(const CSSPropertyID id,
                                         const lynx::tasm::CSSValue &value) {
  if (value.IsEmpty()) {
    return "";
  } else if (value.IsString()) {
    return value.AsString();
  } else if (value.IsNumber()) {
    return CSSValueNumberToString(id, value);
  } else if (value.IsPx()) {
    return NumberToString(value.AsNumber()) + std::string("px");
  } else if (value.IsRpx()) {
    return NumberToString(value.AsNumber()) + std::string("rpx");
  } else if (value.IsPPx()) {
    return NumberToString(value.AsNumber()) + std::string("ppx");
  } else if (value.IsRem()) {
    return NumberToString(value.AsNumber()) + std::string("rem");
  } else if (value.IsEm()) {
    return NumberToString(value.AsNumber()) + std::string("em");
  } else if (value.IsVh()) {
    return NumberToString(value.AsNumber()) + std::string("vh");
  } else if (value.IsVw()) {
    return NumberToString(value.AsNumber()) + std::string("vw");
  } else if (value.IsPercent()) {
    return NumberToString(value.AsNumber()) + std::string("%");
  } else if (value.IsSp()) {
    return NumberToString(value.AsNumber()) + std::string("sp");
  } else if (value.IsCalc()) {
    return value.AsString();
  } else if (value.IsIntrinsic()) {
    return value.GetValue().StdString();
  } else if (value.IsEnv()) {
    return value.AsString();
  } else if (value.IsArray()) {
    return CSSValueArrayToString(id, value);
  } else if (value.IsMap()) {
    // TODO(liyanbo): de parser map type.
    return value.AsJsonString();
  } else if (value.IsEnum()) {
    return CSSValueEnumToString(id, value);
  } else if (value.IsBoolean()) {
    return value.AsBool() ? "true" : "false";
  } else {
    LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE,
                "lynx css decoder unknown value type: %s",
                CSSProperty::GetPropertyName(id).c_str());
    return "";
  }
}

std::string CSSDecoder::CSSValueEnumToString(
    const CSSPropertyID id, const lynx::tasm::CSSValue &value) {
  switch (id) {
    case lynx::tasm::kPropertyIDTop:
    case lynx::tasm::kPropertyIDLeft:
    case lynx::tasm::kPropertyIDRight:
    case lynx::tasm::kPropertyIDBottom:
    case lynx::tasm::kPropertyIDBorderWidth:
    case lynx::tasm::kPropertyIDBorderLeftWidth:
    case lynx::tasm::kPropertyIDBorderRightWidth:
    case lynx::tasm::kPropertyIDBorderTopWidth:
    case lynx::tasm::kPropertyIDBorderBottomWidth:
    case lynx::tasm::kPropertyIDBorderInlineStartWidth:
    case lynx::tasm::kPropertyIDBorderInlineEndWidth:
    case lynx::tasm::kPropertyIDHeight:
    case lynx::tasm::kPropertyIDWidth:
    case lynx::tasm::kPropertyIDMaxWidth:
    case lynx::tasm::kPropertyIDMinWidth:
    case lynx::tasm::kPropertyIDMaxHeight:
    case lynx::tasm::kPropertyIDMinHeight:
    case lynx::tasm::kPropertyIDPadding:
    case lynx::tasm::kPropertyIDPaddingLeft:
    case lynx::tasm::kPropertyIDPaddingRight:
    case lynx::tasm::kPropertyIDPaddingTop:
    case lynx::tasm::kPropertyIDPaddingBottom:
    case lynx::tasm::kPropertyIDPaddingInlineStart:
    case lynx::tasm::kPropertyIDPaddingInlineEnd:
    case lynx::tasm::kPropertyIDMargin:
    case lynx::tasm::kPropertyIDMarginLeft:
    case lynx::tasm::kPropertyIDMarginRight:
    case lynx::tasm::kPropertyIDMarginTop:
    case lynx::tasm::kPropertyIDMarginBottom:
    case lynx::tasm::kPropertyIDMarginInlineStart:
    case lynx::tasm::kPropertyIDMarginInlineEnd:
    case lynx::tasm::kPropertyIDFlexBasis:
    case lynx::tasm::kPropertyIDFontSize:
    case lynx::tasm::kPropertyIDLetterSpacing:
    case lynx::tasm::kPropertyIDLineHeight:
    case lynx::tasm::kPropertyIDLineSpacing:
    case lynx::tasm::kPropertyIDAdaptFontSize:
    case lynx::tasm::kPropertyIDOutlineWidth:
      return ToLengthType(value.GetEnum<LengthValueType>());
    case lynx::tasm::kPropertyIDDisplay:
      return ToDisplayType(value.GetEnum<DisplayType>());
    case lynx::tasm::kPropertyIDOverflow:
    case lynx::tasm::kPropertyIDOverflowX:
    case lynx::tasm::kPropertyIDOverflowY:
      return ToOverflowType(value.GetEnum<OverflowType>());
    case lynx::tasm::kPropertyIDFlexDirection:
      return ToFlexDirectionType(value.GetEnum<FlexDirectionType>());
    case lynx::tasm::kPropertyIDFlexWrap:
      return ToFlexWrapType(value.GetEnum<FlexWrapType>());
    case lynx::tasm::kPropertyIDAlignItems:
    case lynx::tasm::kPropertyIDAlignSelf:
      return ToFlexAlignType(value.GetEnum<FlexAlignType>());
    case lynx::tasm::kPropertyIDAlignContent:
      return ToAlignContentType(value.GetEnum<AlignContentType>());
    case lynx::tasm::kPropertyIDJustifyContent:
      return ToJustifyContentType(value.GetEnum<JustifyContentType>());
    case lynx::tasm::kPropertyIDLinearOrientation:
      return ToLinearOrientationType(value.GetEnum<LinearOrientationType>());
    case lynx::tasm::kPropertyIDLinearGravity:
      return ToLinearGravityType(value.GetEnum<LinearGravityType>());
    case lynx::tasm::kPropertyIDLinearLayoutGravity:
      return ToLinearLayoutGravityType(
          value.GetEnum<LinearLayoutGravityType>());
    case lynx::tasm::kPropertyIDLayoutAnimationCreateTimingFunction:
    case lynx::tasm::kPropertyIDLayoutAnimationDeleteTimingFunction:
    case lynx::tasm::kPropertyIDLayoutAnimationUpdateTimingFunction:
    case lynx::tasm::kPropertyIDTransitionTimingFunction:
      return ToTimingFunctionType(value.GetEnum<TimingFunctionType>());
    case lynx::tasm::kPropertyIDLayoutAnimationCreateProperty:
    case lynx::tasm::kPropertyIDLayoutAnimationDeleteProperty:
    case lynx::tasm::kPropertyIDTransitionProperty:
      return ToAnimationPropertyType(value.GetEnum<AnimationPropertyType>());
    case lynx::tasm::kPropertyIDVisibility:
      return ToVisibilityType(value.GetEnum<VisibilityType>());
    case lynx::tasm::kPropertyIDBorderLeftStyle:
    case lynx::tasm::kPropertyIDBorderRightStyle:
    case lynx::tasm::kPropertyIDBorderTopStyle:
    case lynx::tasm::kPropertyIDBorderBottomStyle:
    case lynx::tasm::kPropertyIDBorderInlineStartStyle:
    case lynx::tasm::kPropertyIDBorderInlineEndStyle:
    case lynx::tasm::kPropertyIDOutlineStyle:
      return ToBorderStyleType(value.GetEnum<BorderStyleType>());
    case lynx::tasm::kPropertyIDDirection:
      return ToDirectionType(value.GetEnum<DirectionType>());
    case lynx::tasm::kPropertyIDRelativeCenter:
      return ToRelativeCenterType(value.GetEnum<RelativeCenterType>());
    case lynx::tasm::kPropertyIDPosition:
      return ToPositionType(value.GetEnum<PositionType>());
    case lynx::tasm::kPropertyIDAnimationFillMode:
      return ToAnimationFillModeType(value.GetEnum<AnimationFillModeType>());
    case lynx::tasm::kPropertyIDAnimationDirection:
      return ToAnimationDirectionType(value.GetEnum<AnimationDirectionType>());
    case lynx::tasm::kPropertyIDAnimationPlayState:
      return ToAnimationPlayStateType(value.GetEnum<AnimationPlayStateType>());
    case lynx::tasm::kPropertyIDWhiteSpace:
      return ToWhiteSpaceType(value.GetEnum<WhiteSpaceType>());
      break;
    case lynx::tasm::kPropertyIDFontWeight:
      return ToFontWeightType(value.GetEnum<FontWeightType>());
      break;
    case lynx::tasm::kPropertyIDWordBreak:
      return ToWordBreakType(value.GetEnum<WordBreakType>());
    case lynx::tasm::kPropertyIDFontStyle:
      return ToFontStyleType(value.GetEnum<FontStyleType>());
      break;
    case lynx::tasm::kPropertyIDTextAlign:
      return ToTextAlignType(value.GetEnum<TextAlignType>());
      break;
    case lynx::tasm::kPropertyIDTextOverflow:
      return ToTextOverflowType(value.GetEnum<TextOverflowType>());
      break;
    case lynx::tasm::kPropertyIDAnimationTimingFunction:
      return ToTimingFunctionType(value.GetEnum<TimingFunctionType>());
    case lynx::tasm::kPropertyIDBoxSizing:
      return ToBoxSizingType(value.GetEnum<BoxSizingType>());
    case lynx::tasm::kPropertyIDLinearCrossGravity:
      return ToLinearCrossGravityType(value.GetEnum<LinearCrossGravityType>());
    case lynx::tasm::kPropertyIDLinearDirection:
      return ToLinearOrientationType(value.GetEnum<LinearOrientationType>());
    case lynx::tasm::kPropertyIDJustifyItems:
    case lynx::tasm::kPropertyIDJustifySelf:
      return ToJustifyType(value.GetEnum<JustifyType>());
    case lynx::tasm::kPropertyIDGridAutoFlow:
      return ToGridAutoFlowType(value.GetEnum<GridAutoFlowType>());
    case lynx::tasm::kPropertyIDImageRendering:
      return ToImageRenderingType(value.GetEnum<ImageRenderingType>());
    case lynx::tasm::kPropertyIDHyphens:
      return ToHyphensType(value.GetEnum<HyphensType>());
    case lynx::tasm::kPropertyIDXAppRegion:
      return ToXAppRegionType(value.GetEnum<XAppRegionType>());
    case lynx::tasm::kPropertyIDXAnimationColorInterpolation:
      return ToXAnimationColorInterpolationType(
          value.GetEnum<XAnimationColorInterpolationType>());
      // TODO(liyanbo): this will support when parser support.
      //    case lynx::tasm::kPropertyIDBackgroundPosition:
      //      break;
      //    case lynx::tasm::kPropertyIDBackgroundOrigin:
      //      break;
      //    case lynx::tasm::kPropertyIDBackgroundRepeat:
      //      break;
      //    case lynx::tasm::kPropertyIDBackgroundSize:
      //      break;
      //    case lynx::tasm::kPropertyIDBackgroundClip:
      //      break;
    default:
      LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE,
                  "lynx css decoder no such enum for css_id: %s",
                  CSSProperty::GetPropertyName(id).c_str());
      return "";
  }
}

std::string CSSDecoder::CSSValueNumberToString(
    const CSSPropertyID id, const lynx::tasm::CSSValue &value) {
  switch (id) {
    case lynx::tasm::kPropertyIDBackgroundColor:
    case lynx::tasm::kPropertyIDBorderLeftColor:
    case lynx::tasm::kPropertyIDBorderRightColor:
    case lynx::tasm::kPropertyIDBorderTopColor:
    case lynx::tasm::kPropertyIDBorderBottomColor:
    case lynx::tasm::kPropertyIDBorderInlineStartColor:
    case lynx::tasm::kPropertyIDBorderInlineEndColor:
    case lynx::tasm::kPropertyIDColor:
    case lynx::tasm::kPropertyIDOutlineColor:
    case lynx::tasm::kPropertyIDTextStrokeColor:
    case lynx::tasm::kPropertyIDXHandleColor:
      return NumberToRGBAColorString(value.AsNumber());
    case lynx::tasm::kPropertyIDLineHeight:
      return NumberToLineHeightString(value.AsNumber());
    default:
      return NumberToString(value.AsNumber());
  }
}

namespace {
void RemoveRedundantZeros(std::string &paramsValue) {
  if (paramsValue.length() <= 1) {
    return;
  }
  while (true) {
    if (paramsValue.back() != '0') {
      break;
    }
    paramsValue.pop_back();
  }
  if (paramsValue.back() == '.') {
    paramsValue.pop_back();
  }
  return;
}

std::string ToLengthWithUnit(const lepus_value &value,
                             lynx::tasm::CSSValuePattern pattern) {
  std::string res;
  std::string paramsValue = std::to_string(static_cast<float>(value.Number()));
  std::string paramsUnit;
  switch (pattern) {
    case lynx::tasm::CSSValuePattern::NUMBER:
      paramsUnit = "";
      break;
    case lynx::tasm::CSSValuePattern::PX:
      paramsUnit = "px";
      break;
    case lynx::tasm::CSSValuePattern::RPX:
      paramsUnit = "rpx";
      break;
    case lynx::tasm::CSSValuePattern::PPX:
      paramsUnit = "ppx";
      break;
    case lynx::tasm::CSSValuePattern::REM:
      paramsUnit = "rem";
      break;
    case lynx::tasm::CSSValuePattern::EM:
      paramsUnit = "em";
      break;
    case lynx::tasm::CSSValuePattern::PERCENT:
      paramsUnit = "%";
      break;
    case lynx::tasm::CSSValuePattern::VH:
      paramsUnit = "vh";
      break;
    case lynx::tasm::CSSValuePattern::VW:
      paramsUnit = "vw";
      break;
    case lynx::tasm::CSSValuePattern::CALC:
    case lynx::tasm::CSSValuePattern::INTRINSIC:
      paramsValue = value.StdString();
      paramsUnit = "";
      break;
    case lynx::tasm::CSSValuePattern::FR:
      paramsUnit = "fr";
      break;
    default:
      LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE,
                  "%d length type pattern is invalid.", (int)pattern);
      break;
  }
  RemoveRedundantZeros(paramsValue);
  res = paramsValue + paramsUnit;
  return res;
}

void GetTransformParams(std::string &length, const lepus_value &value,
                        const lepus_value &unit) {
  auto pattern = static_cast<lynx::tasm::CSSValuePattern>(unit.Number());
  length = ToLengthWithUnit(value, pattern);
  return;
}

std::string GetTransformMatrixParams(fml::RefPtr<lepus::CArray> &arr,
                                     int count) {
  std::ostringstream oss;
  for (int i = 1; i < count; ++i) {
    if (i > 1) {
      oss << ", ";
    }
    oss << arr->get(i).Number();
  }
  return oss.str();
};
}  // namespace

std::string CSSDecoder::ToTransformType(lynx::starlight::TransformType type) {
  switch (type) {
    case TransformType::kNone:
      return "none";
    case TransformType::kTranslate:
      return "translate";
    case TransformType::kTranslateX:
      return "translateX";
    case TransformType::kTranslateY:
      return "translateY";
    case TransformType::kTranslateZ:
      return "translateZ";
    case TransformType::kTranslate3d:
      return "translate3d";
    case TransformType::kRotate:
      return "rotate";
    case TransformType::kRotateX:
      return "rotateX";
    case TransformType::kRotateY:
      return "rotateY";
    case TransformType::kRotateZ:
      return "rotateZ";
    case TransformType::kScale:
      return "scale";
    case TransformType::kScaleX:
      return "scaleX";
    case TransformType::kScaleY:
      return "scaleY";
    case TransformType::kSkew:
      return "skew";
    case TransformType::kSkewX:
      return "skewX";
    case TransformType::kSkewY:
      return "skewY";
    case TransformType::kMatrix:
      return "matrix";
    case TransformType::kMatrix3d:
      return "matrix3d";
    default:
      LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE,
                  "%d transform type is invalid.", (int)type);
      return "";
  }
}

namespace {
std::string ToTransformProperty(const lynx::tasm::CSSValue &value) {
  std::string res;
  auto items = value.GetValue().Array();
  for (size_t i = 0; i < items->size(); i++) {
    auto arr = items->get(i).Array();
    TransformType type = static_cast<TransformType>(
        arr->get(TransformRawData::INDEX_FUNC).Number());
    std::string func = CSSDecoder::ToTransformType(type);
    std::string param1 = "0";
    std::string param2 = "0";
    std::string param3 = "0";
    switch (type) {
      case TransformType::kTranslate:
        GetTransformParams(param1,
                           arr->get(TransformRawData::INDEX_TRANSLATE_0),
                           arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT));
        if (arr->size() <= TransformRawData::INDEX_TRANSLATE_1) {
          param2 = param1;
        } else {
          GetTransformParams(
              param2, arr->get(TransformRawData::INDEX_TRANSLATE_1),
              arr->get(TransformRawData::INDEX_TRANSLATE_1_UNIT));
        }
        func += std::string("(" + param1 + "," + param2 + ")");
        break;
      case TransformType::kTranslateX:
      case TransformType::kTranslateY:
      case TransformType::kTranslateZ:
        GetTransformParams(param1,
                           arr->get(TransformRawData::INDEX_TRANSLATE_0),
                           arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT));
        func += std::string("(" + param1 + ")");
        break;
      case TransformType::kTranslate3d:
        GetTransformParams(param1,
                           arr->get(TransformRawData::INDEX_TRANSLATE_0),
                           arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT));
        GetTransformParams(param2,
                           arr->get(TransformRawData::INDEX_TRANSLATE_1),
                           arr->get(TransformRawData::INDEX_TRANSLATE_1_UNIT));
        GetTransformParams(param3,
                           arr->get(TransformRawData::INDEX_TRANSLATE_2),
                           arr->get(TransformRawData::INDEX_TRANSLATE_2_UNIT));
        func += std::string("(" + param1 + "," + param2 + "," + param3 + ")");
        break;
      case TransformType::kRotate:
      case TransformType::kRotateX:
      case TransformType::kRotateY:
      case TransformType::kRotateZ:
      case TransformType::kSkewX:
      case TransformType::kSkewY:
        param1 = std::to_string(
            arr->get(TransformRawData::INDEX_ROTATE_ANGLE).Number());
        RemoveRedundantZeros(param1);
        func += std::string("(" + param1 + "deg" + ")");
        break;
      case TransformType::kSkew:
        param1 =
            std::to_string(arr->get(TransformRawData::INDEX_SKEW_0).Number());
        RemoveRedundantZeros(param1);
        if (arr->size() <= TransformRawData::INDEX_SKEW_1) {
          param2 = param1;
        } else {
          param2 =
              std::to_string(arr->get(TransformRawData::INDEX_SKEW_1).Number());
        }
        RemoveRedundantZeros(param2);
        func += std::string("(" + param1 + "deg, " + param2 + "deg)");
        break;
      case TransformType::kScale:
        param1 =
            std::to_string(arr->get(TransformRawData::INDEX_SCALE_0).Number());
        if (arr->size() <= TransformRawData::INDEX_SCALE_1) {
          param2 = param1;
        } else {
          param2 = std::to_string(
              arr->get(TransformRawData::INDEX_SCALE_1).Number());
        }
        RemoveRedundantZeros(param1);
        RemoveRedundantZeros(param2);
        func += std::string("(" + param1 + "," + param2 + ")");
        break;
      case TransformType::kScaleX:
      case TransformType::kScaleY:
        param1 =
            std::to_string(arr->get(TransformRawData::INDEX_SCALE_0).Number());
        RemoveRedundantZeros(param1);
        func += std::string("(" + param1 + ")");
        break;
      case TransformType::kMatrix:
      case TransformType::kMatrix3d:
        func += std::string(
            "(" +
            GetTransformMatrixParams(
                arr, type == starlight::TransformType::kMatrix ? 7 : 17) +
            ")");
        break;
      default:
        break;
    }
    res += func;
    res += " ";
  }
  if (!res.empty()) {
    res.pop_back();
  }
  return res;
}

std::string TimingFunctionToString(const lynx::lepus::Value &value) {
  std::string time_func = "";
  std::string comma = ",";
  std::string func_params = "";
  if (value.IsNumber()) {
    TimingFunctionType timing_func =
        static_cast<TimingFunctionType>(value.Number());
    time_func = CSSDecoder::ToTimingFunctionType(timing_func);
  } else {
    auto arr = value.Array();
    TimingFunctionType timing_func = static_cast<TimingFunctionType>(
        arr->get(TimingFunctionData::INDEX_TYPE).Number());
    time_func = CSSDecoder::ToTimingFunctionType(timing_func);
    if (timing_func == TimingFunctionType::kSquareBezier) {
      float x1 = arr->get(TimingFunctionData::INDEX_X1).Number();
      float y1 = arr->get(TimingFunctionData::INDEX_Y1).Number();
      std::string paramx1 = std::to_string(x1);
      std::string paramy1 = std::to_string(y1);
      RemoveRedundantZeros(paramx1);
      RemoveRedundantZeros(paramy1);
      func_params = "(" + paramx1 + comma + paramy1 + ")";
    } else if (timing_func == TimingFunctionType::kCubicBezier) {
      float x1 = arr->get(TimingFunctionData::INDEX_X1).Number();
      float y1 = arr->get(TimingFunctionData::INDEX_Y1).Number();
      float x2 = arr->get(TimingFunctionData::INDEX_X2).Number();
      float y2 = arr->get(TimingFunctionData::INDEX_Y2).Number();
      std::string paramx1 = std::to_string(x1);
      std::string paramy1 = std::to_string(y1);
      std::string paramx2 = std::to_string(x2);
      std::string paramy2 = std::to_string(y2);
      RemoveRedundantZeros(paramx1);
      RemoveRedundantZeros(paramy1);
      RemoveRedundantZeros(paramx2);
      RemoveRedundantZeros(paramy2);
      func_params = "(" + paramx1 + comma + paramy1 + comma + paramx2 + comma +
                    paramy2 + ")";
    } else {
      std::string error_msg =
          std::string("does not support such time func implementation") +
          std::to_string(static_cast<int>(timing_func));
      LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE, error_msg.c_str());
    }
    time_func += func_params;
  }
  return time_func;
}

std::string ToTransitionProperties(const lynx::tasm::CSSValue &value) {
  std::string res = "[";
  std::string split = ", ";
  auto group = value.GetValue().Array();
  for (size_t i = 0; i < group->size(); ++i) {
    AnimationPropertyType transition_property =
        static_cast<AnimationPropertyType>(group->get(i).Number());
    std::string property_string =
        CSSDecoder::ToAnimationPropertyType(transition_property);
    res += property_string;
    if (i != group->size() - 1) {
      res += split;
    }
  }
  res += "]";
  return res;
}

std::string ToTimingFunctions(const lynx::tasm::CSSValue &value) {
  std::string res = "[";
  std::string split = ", ";
  auto group = value.GetValue().Array();
  for (size_t i = 0; i < group->size(); ++i) {
    std::string timing_function_string = TimingFunctionToString(group->get(i));
    res += timing_function_string;
    if (i != group->size() - 1) {
      res += split;
    }
  }
  res += "]";
  return res;
}

std::string ToTransitionProperty(const lynx::tasm::CSSValue &value) {
  std::string res = "";
  std::string space = " ";
  auto group = value.GetValue().Array();
  BASE_STATIC_STRING_DECL(kProperty, "property");
  BASE_STATIC_STRING_DECL(kDuration, "duration");
  BASE_STATIC_STRING_DECL(kTiming, "timing");
  BASE_STATIC_STRING_DECL(kDelay, "delay");
  for (size_t i = 0; i < group->size(); i++) {
    auto dict = group->get(i).Table();
    AnimationPropertyType animation_property =
        static_cast<AnimationPropertyType>(dict->GetValue(kProperty).Number());
    std::string property =
        CSSDecoder::ToAnimationPropertyType(animation_property);
    res += property;

    long duration = dict->GetValue(kDuration).Number();
    res += space;
    std::string duration_time = std::to_string(duration) + "ms";
    res += duration_time;

    if (dict->Contains(kTiming)) {
      DCHECK(dict->GetValue(kTiming).IsArray());
      std::string time_func =
          TimingFunctionToString(dict->GetValue(kTiming).Array()->get(0));
      res += space;
      res += time_func;
    }

    if (dict->Contains(kDelay)) {
      long delay = dict->GetValue(kDelay).Number();
      std::string delay_time = std::to_string(delay) + "ms";
      res += space;
      res += delay_time;
    }
    res += ",";
  }
  if (!res.empty()) {
    res.pop_back();
  }
  return res;
}

std::string ToVerticalAlignProperty(const lynx::tasm::CSSValue &value) {
  auto vertical_align_array = value.GetValue().Array();
  VerticalAlignType type =
      static_cast<VerticalAlignType>(vertical_align_array->get(0).Int32());
  switch (type) {
    case VerticalAlignType::kBaseline:
      return "baseline";
    case VerticalAlignType::kSub:
      return "sub";
    case VerticalAlignType::kSuper:
      return "super";
    case VerticalAlignType::kTop:
      return "top";
    case VerticalAlignType::kBottom:
      return "bottom";
    case VerticalAlignType::kTextTop:
      return "text-top";
    case VerticalAlignType::kTextBottom:
      return "text-bottom";
    case VerticalAlignType::kMiddle:
      return "middle";
    case VerticalAlignType::kCenter:
      return "center";
    case VerticalAlignType::kPercent:
    case VerticalAlignType::kLength:
      return ToLengthWithUnit(
          vertical_align_array->get(2),
          static_cast<CSSValuePattern>(vertical_align_array->get(3).Int32()));
    default:
      return "0px";
  }
}

std::string ToXAutoFontSizeProperty(const lynx::tasm::CSSValue &value) {
  std::string res;
  auto auto_font_size_array = value.GetValue().Array();
  bool is_auto_font_size = auto_font_size_array->get(0).Bool();
  res += is_auto_font_size ? "true" : "false";

  double min_font_size = auto_font_size_array->get(1).Number();
  if (min_font_size != 0.f) {
    res += " ";
    res += ToLengthWithUnit(
        auto_font_size_array->get(1),
        static_cast<CSSValuePattern>(auto_font_size_array->get(2).Int32()));
  }

  double max_font_size = auto_font_size_array->get(3).Number();
  if (max_font_size != 0.f) {
    res += " ";
    res += ToLengthWithUnit(
        auto_font_size_array->get(3),
        static_cast<CSSValuePattern>(auto_font_size_array->get(4).Int32()));
  }

  double step_granularity = auto_font_size_array->get(5).Number();
  if (step_granularity != 1.f) {
    res += " ";
    res += ToLengthWithUnit(
        auto_font_size_array->get(5),
        static_cast<CSSValuePattern>(auto_font_size_array->get(6).Int32()));
  }

  return res;
}

std::string ToXAutoFontSizePresetSizesProperty(
    const lynx::tasm::CSSValue &value) {
  std::string res;
  auto preset_sizes_array = value.GetValue().Array();
  for (size_t i = 0; i < preset_sizes_array->size(); i = i + 2) {
    res += CSSDecoder::CSSValueToString(
        kPropertyIDXAutoFontSizePresetSizes,
        CSSValue(preset_sizes_array->get(i),
                 static_cast<CSSValuePattern>(
                     preset_sizes_array->get(i + 1).Int32())));
    if (i != preset_sizes_array->size() - 2) {
      res += " ";
    }
  }
  return res;
}

void ComputeShadowStyle(std::string &prop_result, const std::string &key,
                        const fml::RefPtr<lynx::lepus::Dictionary> &dict) {
  auto prop_arr = dict->GetValue(key).Array();
  GetTransformParams(prop_result, prop_arr->get(0), prop_arr->get(1));
}

std::string ToBoxShadowProperty(const lynx::tasm::CSSValue &value) {
  std::string res = "";
  std::string space = " ";
  std::string comma = ",";
  BASE_STATIC_STRING_DECL(kEnable, "enable");
  BASE_STATIC_STRING_DECL(kBlur, "blur");
  BASE_STATIC_STRING_DECL(kSpread, "spread");
  BASE_STATIC_STRING_DECL(kOption, "option");
  BASE_STATIC_STRING_DECL(kColor, "color");
  auto group = value.GetValue().Array();
  for (size_t i = 0; i < group->size(); i++) {
    auto dict = group->get(i).Table();
    bool enable = true;
    if (dict->Contains(kEnable)) {
      enable = dict->GetValue(kEnable).Bool();
    }
    if (enable) {
      std::string h_length = "0";
      std::string v_length = "0";
      ComputeShadowStyle(h_length, "h_offset", dict);
      ComputeShadowStyle(v_length, "v_offset", dict);
      res += (h_length + space + v_length + space);

      if (dict->Contains(kBlur)) {
        std::string blur_length = "0";
        ComputeShadowStyle(blur_length, kBlur.str(), dict);
        res += (blur_length + space);
      }

      if (dict->Contains(kSpread)) {
        std::string spread_length = "0";
        ComputeShadowStyle(spread_length, kSpread.str(), dict);
        res += (spread_length + space);
      }

      if (dict->Contains(kOption)) {
        auto option = dict->GetValue(kOption).Number();
        std::string option_type =
            CSSDecoder::ToShadowOption(static_cast<ShadowOption>(option));
        res += (option_type + space);
      }

      if (dict->Contains(kColor)) {
        auto color = dict->GetValue(kColor).Number();
        res += (NumberToRGBAColorString(color) + space);
      }

      if (res.length() == 0) {
        res += space;
      } else {
        res.pop_back();
      }
    } else {
      res += ("none" + space);
    }
    res += comma;
  }
  if (!res.empty()) {
    res.pop_back();
  }
  return res;
}

std::string ToBorderRadiusProperty(const lynx::tasm::CSSValue &value) {
  auto container = value.GetValue().Array();
  std::string radius_result = "";
  std::string space = " ";
  std::string res = "";
  std::string horizontal_res = "";
  std::string vertical_res = "";
  bool is_all_same = true;

  for (int i = 0; i < 4; i++) {
    auto parse_result = ToLengthWithUnit(
        container->get(i * 4),
        static_cast<CSSValuePattern>(container->get(i * 4 + 1).Number()));
    std::string temp = std::move(parse_result);
    if (radius_result.empty()) {
      radius_result = temp;
    } else {
      is_all_same &= (radius_result == temp);
    }
    horizontal_res += temp + space;

    parse_result = ToLengthWithUnit(
        container->get(i * 4 + 2),
        static_cast<CSSValuePattern>(container->get(i * 4 + 3).Number()));
    temp = std::move(parse_result);
    if (radius_result.empty()) {
      radius_result = temp;
    } else {
      is_all_same &= (radius_result == temp);
    }
    vertical_res += temp + space;
  }
  if (horizontal_res == vertical_res) {
    res = horizontal_res;
  } else {
    res = horizontal_res + "/" + vertical_res;
  }
  if (is_all_same) {
    res = radius_result;
  } else if (!res.empty()) {
    res.pop_back();
  }
  return res;
}

std::string ToSingleBorderRadiusProperty(const lynx::tasm::CSSValue &value) {
  auto arr = value.GetValue().Array();
  std::string res = "";
  std::string space = " ";
  auto parse_result = ToLengthWithUnit(
      arr->get(0), static_cast<CSSValuePattern>(arr->get(1).Number()));
  std::string radiusX = std::move(parse_result);
  res += radiusX + space;
  parse_result = ToLengthWithUnit(
      arr->get(2), static_cast<CSSValuePattern>(arr->get(3).Number()));
  std::string radiusY = std::move(parse_result);
  if (radiusX == radiusY) {
    res.pop_back();
  } else {
    res += radiusY;
  }
  return res;
}

std::string ToBackgroundSizeEnumString(int32_t val) {
  constexpr int32_t BACKGROUND_SIZE_AUTO =
      -static_cast<int>(starlight::BackgroundSizeType::kAuto);
  constexpr int32_t BACKGROUND_SIZE_COVER =
      -static_cast<int>(starlight::BackgroundSizeType::kCover);
  constexpr int32_t BACKGROUND_SIZE_CONTAIN =
      -static_cast<int>(starlight::BackgroundSizeType::kContain);
  if (val == BACKGROUND_SIZE_AUTO) {
    return "auto";
  } else if (val == BACKGROUND_SIZE_COVER) {
    return "cover";
  } else if (val == BACKGROUND_SIZE_CONTAIN) {
    return "contain";
  } else {
    return "";
  }
}

std::string ToBackgroundSizeProperty(const lynx::tasm::CSSValue &value) {
  auto arr = value.GetValue().Array();
  std::string res = "";
  std::string space = " ";
  std::string comma = ",";
  for (size_t i = 0; i != arr->size(); ++i) {
    auto array = arr->get(i).Array();
    if (array->size() != 4) {
      return res;
    }
    std::string parse_result = "";
    auto pattern = static_cast<CSSValuePattern>(array->get(0).Number());
    if (pattern != CSSValuePattern::NUMBER) {
      parse_result = ToLengthWithUnit(array->get(1), pattern);
    } else {
      auto val = static_cast<int32_t>(array->get(1).Number());
      parse_result = ToBackgroundSizeEnumString(val);
    }
    // contain and cover can only be setted once for one background-size
    if (parse_result != "contain" && parse_result != "cover") {
      res += parse_result;
      res += space;
    }

    pattern = static_cast<CSSValuePattern>(array->get(2).Number());
    if (pattern != CSSValuePattern::NUMBER) {
      parse_result = ToLengthWithUnit(array->get(3), pattern);
    } else {
      auto val = static_cast<int32_t>(array->get(3).Number());
      parse_result = ToBackgroundSizeEnumString(val);
    }
    res += parse_result;
    res += comma;
  }
  if (!res.empty()) {
    res.pop_back();
  }
  return res;
}

std::string ToBackgroundOriginProperty(const lynx::tasm::CSSValue &value) {
  std::string res = "";
  auto arr = value.GetValue().Array();
  for (size_t i = 0; i < arr->size(); i++) {
    if (i != 0) {
      res += " ,";
    }
    int32_t raw_value = static_cast<int32_t>(arr->get(i).Number());
    auto enum_value = static_cast<starlight::BackgroundOriginType>(raw_value);
    switch (enum_value) {
      case starlight::BackgroundOriginType::kPaddingBox:
        res += "padding-box";
        break;
      case starlight::BackgroundOriginType::kContentBox:
        res += "content-box";
        break;
      case starlight::BackgroundOriginType::kBorderBox:
        res += "border-box";
        break;
    }
  }

  return res;
}

std::string BackgroundPositionEnumToString(uint32_t value) {
  auto type = static_cast<starlight::BackgroundPositionType>(value);
  switch (type) {
    case starlight::BackgroundPositionType::kTop:
      return "top";
    case starlight::BackgroundPositionType::kRight:
      return "right";
    case starlight::BackgroundPositionType::kBottom:
      return "bottom";
    case starlight::BackgroundPositionType::kLeft:
      return "left";
    case starlight::BackgroundPositionType::kCenter:
      return "center";
    default:
      return "";
  }
}

std::string ToBackgroundPositionProperty(const lynx::tasm::CSSValue &value) {
  std::string res = "";
  auto arr = value.GetValue().Array();
  for (size_t i = 0; i < arr->size(); i++) {
    if (i != 0) {
      res += ", ";
    }

    if (!arr->get(i).IsArray()) {
      // some thing is error
      break;
    }

    auto pos_arr = arr->get(i).Array();
    if (pos_arr->size() != 4) {
      break;
    }

    uint32_t enum_begin =
        static_cast<uint32_t>(starlight::BackgroundPositionType::kTop);
    uint32_t enum_end =
        static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter);

    // position x
    uint32_t x_pattern = pos_arr->get(0).UInt32();
    if (x_pattern >= enum_begin && x_pattern <= enum_end) {
      res += BackgroundPositionEnumToString(x_pattern);
    } else {
      res += ToLengthWithUnit(pos_arr->get(1),
                              static_cast<CSSValuePattern>(x_pattern));
    }
    // space split
    res += " ";
    // position y
    uint32_t y_pattern = pos_arr->get(2).UInt32();
    if (y_pattern >= enum_begin && y_pattern <= enum_end) {
      res += BackgroundPositionEnumToString(y_pattern);
    } else {
      res += ToLengthWithUnit(pos_arr->get(3),
                              static_cast<CSSValuePattern>(y_pattern));
    }
  }

  return res;
}

std::string BackgroundRepeatEnumToString(uint32_t raw_value) {
  auto rep = static_cast<starlight::BackgroundRepeatType>(raw_value);
  switch (rep) {
    case starlight::BackgroundRepeatType::kRepeat:
      return "repeat";
    case starlight::BackgroundRepeatType::kNoRepeat:
      return "no-repeat";
    case starlight::BackgroundRepeatType::kRepeatX:
      return "repeat-x";
    case starlight::BackgroundRepeatType::kRepeatY:
      return "repeat-y";
    case starlight::BackgroundRepeatType::kRound:
      return "round";
    case starlight::BackgroundRepeatType::kSpace:
      return "space";
    default:
      return "";
  }
}

std::string ToBackgroundRepeatProperty(const lynx::tasm::CSSValue &value) {
  std::string res = "";
  auto arr = value.GetValue().Array();
  for (size_t i = 0; i < arr->size(); i++) {
    if (i != 0) {
      res += ", ";
    }

    if (!arr->get(i).IsArray()) {
      // some thing is error
      break;
    }

    auto rep_arr = arr->get(i).Array();
    if (rep_arr->size() != 2) {
      break;
    }

    // repeat x
    res += BackgroundRepeatEnumToString(rep_arr->get(0).UInt32());
    if (rep_arr->get(0).UInt32() <=
        3) {  // space and round only have one keyword
      // split
      res += " ";
      res += BackgroundRepeatEnumToString(rep_arr->get(1).UInt32());
    }
  }

  return res;
}

std::string BackgroundUrlToString(const lynx::lepus::Value &value) {
  if (!value.IsString()) {
    return "";
  }

  std::string res = "url(\"";

  res += value.StdString();

  res += "\")";

  return res;
}

std::string GradientColorStopToString(const lynx::lepus::CArray *colors,
                                      const lynx::lepus::CArray *pos) {
  std::string res = "";

  if (pos->size() > 0 && pos->size() != colors->size()) {
    return res;
  }

  for (size_t i = 0; i < colors->size(); i++) {
    auto number = colors->get(i).Number();
    if (number != 0.0) {
      res += NumberToRGBAColorString(number);
    } else {
      res += "transparent";
    }

    if (pos->size() > 0) {
      res += " ";
      res += NumberToString(pos->get(i).Number());
      res += "%";
    }

    if (i < colors->size() - 1) {
      res += " ,";
    }
  }

  return res;
}

std::string BackgroundLinearGradientToString(const lynx::lepus::Value &value) {
  if (!value.IsArray()) {
    return "";
  }

  auto arr = value.Array();
  if (arr->size() < 3) {
    return "";
  }

  auto angle = arr->get(0);
  auto colors = arr->get(1);
  auto pos = arr->get(2);

  if (!colors.IsArray() || !pos.IsArray()) {
    return "";
  }

  std::string res = "linear-gradient(";
  // parse angle
  res += NumberToString(angle.Number());
  res += "deg ,";

  res += GradientColorStopToString(colors.Array().get(), pos.Array().get());

  res += ")";

  return res;
}

std::string BackgroundRadialGradientToString(const lynx::lepus::Value &value) {
  if (!value.IsArray()) {
    return "";
  }

  auto arr = value.Array();
  if (arr->size() != 3) {
    return "";
  }

  auto shape_size = arr->get(0);
  auto colors = arr->get(1);
  auto pos = arr->get(2);

  if (!shape_size.IsArray() || !colors.IsArray() || !pos.IsArray()) {
    return "";
  }

  std::string res = "radial-gradient(";

  // shape size
  auto shape_arr = shape_size.Array();
  auto shape_type =
      static_cast<RadialGradientShapeType>(shape_arr->get(0).UInt32());
  if (shape_type == starlight::RadialGradientShapeType::kEllipse) {
    res += "ellipse ";
  } else {
    res += "circle ";
  }

  auto shape_size_type =
      static_cast<RadialGradientSizeType>(shape_arr->get(1).UInt32());
  switch (shape_size_type) {
    case starlight::RadialGradientSizeType::kClosestCorner:
      res += "closest-corner ";
      break;
    case starlight::RadialGradientSizeType::kClosestSide:
      res += "closest-side ";
      break;
    case starlight::RadialGradientSizeType::kFarthestCorner:
      res += "farthest-corner ";
      break;
    case starlight::RadialGradientSizeType::kFarthestSide:
      res += "farthest-side ";
      break;
    case starlight::RadialGradientSizeType::kLength:
      res += ToLengthWithUnit(shape_arr->get(7),
                              static_cast<lynx::tasm::CSSValuePattern>(
                                  shape_arr->get(6).Number())) +
             " ";
      res += ToLengthWithUnit(shape_arr->get(9),
                              static_cast<lynx::tasm::CSSValuePattern>(
                                  shape_arr->get(8).Number())) +
             " ";
      break;
    default:
      res += " ";
      break;
  }

  // center position
  res += "at ";
  auto pos_x_type = shape_arr->get(2).UInt32();
  auto pos_x_value = shape_arr->get(3);
  auto pos_y_type = shape_arr->get(4).UInt32();
  auto pos_y_value = shape_arr->get(5);

  uint32_t enum_begin =
      static_cast<uint32_t>(starlight::BackgroundPositionType::kTop);
  uint32_t enum_end =
      static_cast<uint32_t>(starlight::BackgroundPositionType::kCenter);

  if (pos_x_type >= enum_begin && pos_x_type <= enum_end) {
    res += BackgroundPositionEnumToString(pos_x_type);
  } else {
    res +=
        ToLengthWithUnit(pos_x_value, static_cast<CSSValuePattern>(pos_x_type));
  }

  res += " ";
  if (pos_y_type >= enum_begin && pos_y_type <= enum_end) {
    res += BackgroundPositionEnumToString(pos_y_type);
  } else {
    res +=
        ToLengthWithUnit(pos_y_value, static_cast<CSSValuePattern>(pos_y_type));
  }

  res += " ,";

  res += GradientColorStopToString(colors.Array().get(), pos.Array().get());

  res += ")";

  return res;
}

std::string ToBackgroundImageProperty(const lynx::tasm::CSSValue &value) {
  std::string res = "";

  auto arr = value.GetValue().Array();

  for (size_t i = 0; i < arr->size(); i++) {
    auto type = arr->get(i);
    if (!type.IsUInt32()) {
      continue;
    }

    i++;

    auto image_value = arr->get(i);

    auto e_type = static_cast<BackgroundImageType>(type.UInt32());

    switch (e_type) {
      case starlight::BackgroundImageType::kNone:
        res += "none";
        break;
      case starlight::BackgroundImageType::kUrl:
        res += BackgroundUrlToString(image_value);
        break;
      case starlight::BackgroundImageType::kLinearGradient:
        res += BackgroundLinearGradientToString(image_value);
        break;
      case starlight::BackgroundImageType::kRadialGradient:
        res += BackgroundRadialGradientToString(image_value);
        break;
    }

    if (i > 0 && i < arr->size() - 1) {
      res += " ,";
    }
  }

  return res;
}

std::string GradientColorToString(const lynx::tasm::CSSValue &value) {
  if (!value.IsArray()) {
    return "";
  }

  auto arr = value.GetValue().Array();
  if (arr->size() != 2) {
    return "";
  }

  auto type = static_cast<starlight::BackgroundImageType>(arr->get(0).UInt32());
  if (type == starlight::BackgroundImageType::kLinearGradient) {
    return BackgroundLinearGradientToString(arr->get(1));
  } else if (type == starlight::BackgroundImageType::kRadialGradient) {
    return BackgroundRadialGradientToString(arr->get(1));
  } else {
    return "";
  }
}

std::string ToGridTrackSizingProperty(const lynx::tasm::CSSValue &value) {
  if (!value.IsArray()) {
    return "";
  }

  auto items = value.GetValue().Array();
  std::string res;

  for (size_t i = 0; i < items->size(); i += 2) {
    if (i != 0) {
      res += " ";
    }

    auto length_value = items->get(i);
    auto pattern_value =
        static_cast<CSSValuePattern>(items->get(i + 1).Number());
    // handle minmax function.
    if (pattern_value == tasm::CSSValuePattern::ENUM &&
        static_cast<tasm::CSSFunctionType>(length_value.Number()) ==
            tasm::CSSFunctionType::MINMAX) {
      res += "minmax(";
      i += 2;
      if (i + 3 >= items->size()) {
        return res;
      }
      res +=
          (static_cast<CSSValuePattern>(items->get(i + 1).Number()) ==
           lynx::tasm::CSSValuePattern::ENUM)
              ? "auto"
              : ToLengthWithUnit(
                    items->get(i),
                    static_cast<CSSValuePattern>(items->get(i + 1).Number()));
      res += ",";
      i += 2;
      res +=
          (static_cast<CSSValuePattern>(items->get(i + 1).Number()) ==
           lynx::tasm::CSSValuePattern::ENUM)
              ? "auto"
              : ToLengthWithUnit(
                    items->get(i),
                    static_cast<CSSValuePattern>(items->get(i + 1).Number()));
      res += ")";
    } else {
      res += (pattern_value == lynx::tasm::CSSValuePattern::ENUM)
                 ? "auto"
                 : ToLengthWithUnit(length_value, pattern_value);
    }
  }
  return res;
}

std::string ToFilterProperty(const lynx::tasm::CSSValue &value) {
  if (!value.IsArray()) {
    return "";
  }
  auto arr = value.GetValue().Array();
  std::string res;
  starlight::FilterType type =
      static_cast<starlight::FilterType>(arr->get(0).Number());
  switch (type) {
    case starlight::FilterType::kGrayscale:
      res =
          "grayscale(" +
          ToLengthWithUnit(
              arr->get(1),
              static_cast<lynx::tasm::CSSValuePattern>(arr->get(2).Number())) +
          ")";
      break;
    case starlight::FilterType::kBlur:
      res +=
          "blur(" +
          ToLengthWithUnit(
              arr->get(1),
              static_cast<lynx::tasm::CSSValuePattern>(arr->get(2).Number())) +
          ")";
      break;
    case starlight::FilterType::kNone:
      res = "none";
    default:
      res = "not support";
      break;
  }
  return res;
}

std::string ToTextDecorationProperty(const lynx::tasm::CSSValue &value) {
  if (!value.IsArray()) {
    return "";
  }
  auto arr = value.GetValue().Array();
  std::string res = "";
  for (size_t i = 0; i < arr->size(); i++) {
    int decoration = static_cast<int>(arr->get(i).Number());
    if (decoration & static_cast<int>(starlight::TextDecorationType::kColor)) {
      uint32_t textDecorationColor =
          static_cast<uint32_t>(arr->get(i + 1).Number());
      lynx::tasm::CSSColor color;
      color.a_ = (textDecorationColor & 0xff000000) >> 24;
      color.r_ = (textDecorationColor & 0x00ff0000) >> 16;
      color.g_ = (textDecorationColor & 0x0000ff00) >> 8;
      color.b_ = textDecorationColor & 0x000000ff;

      std::stringstream color_string;
      if ((static_cast<unsigned int>(color.a_)) != 255) {
        color_string << "rgba(" << static_cast<unsigned int>(color.r_) << ","
                     << static_cast<unsigned int>(color.g_) << ","
                     << static_cast<unsigned int>(color.b_) << ","
                     << color.a_ / 255 << ")";
      } else {
        color_string << "rgb(" << static_cast<unsigned int>(color.r_) << ","
                     << static_cast<unsigned int>(color.g_) << ","
                     << static_cast<unsigned int>(color.b_) << ")";
      }
      res += " ";
      res += color_string.str();
      i++;
      continue;
    }
    if (decoration &
        static_cast<int>(starlight::TextDecorationType::kLineThrough)) {
      res += " line-through";
    } else if (decoration &
               static_cast<int>(starlight::TextDecorationType::kUnderLine)) {
      res += " underline";
    } else if (decoration) {
      switch (decoration) {
        case static_cast<int>(starlight::TextDecorationType::kSolid):
          res += " solid";
          break;
        case static_cast<int>(starlight::TextDecorationType::kDouble):
          res += " double";
          break;
        case static_cast<int>(starlight::TextDecorationType::kDotted):
          res += " dotted";
          break;
        case static_cast<int>(starlight::TextDecorationType::kDashed):
          res += " dashed";
          break;
        case static_cast<int>(starlight::TextDecorationType::kWavy):
          res += " wavy";
          break;
        default:
          break;
      }
    }
  }
  if (res.empty()) {
    res = "none";
  } else {
    res = res.substr(1);  // trim the first whitespace
  }
  return res;
}
}  // namespace

std::string CSSDecoder::CSSValueArrayToString(
    const CSSPropertyID id, const lynx::tasm::CSSValue &value) {
  switch (id) {
    case kPropertyIDAnimationTimingFunction:
    case kPropertyIDTransitionTimingFunction:
      return ToTimingFunctions(value);
    case kPropertyIDTransform:
      return ToTransformProperty(value);
    case kPropertyIDTransition:
      return ToTransitionProperty(value);
    case kPropertyIDTransitionProperty:
      return ToTransitionProperties(value);
    case kPropertyIDBoxShadow:
    case kPropertyIDTextShadow:
      return ToBoxShadowProperty(value);
    case kPropertyIDBorderRadius:
      return ToBorderRadiusProperty(value);
    case kPropertyIDBorderTopLeftRadius:
    case kPropertyIDBorderTopRightRadius:
    case kPropertyIDBorderBottomLeftRadius:
    case kPropertyIDBorderBottomRightRadius:
    case kPropertyIDBorderStartStartRadius:
    case kPropertyIDBorderStartEndRadius:
    case kPropertyIDBorderEndStartRadius:
    case kPropertyIDBorderEndEndRadius:
      return ToSingleBorderRadiusProperty(value);
    case kPropertyIDBackgroundSize:
      return ToBackgroundSizeProperty(value);
    case kPropertyIDBackgroundOrigin:
      return ToBackgroundOriginProperty(value);
    case kPropertyIDBackgroundPosition:
      return ToBackgroundPositionProperty(value);
    case kPropertyIDBackgroundRepeat:
      return ToBackgroundRepeatProperty(value);
    case kPropertyIDBackgroundImage:
      return ToBackgroundImageProperty(value);
    case kPropertyIDColor:
      return GradientColorToString(value);
    case kPropertyIDTextDecoration:
      return ToTextDecorationProperty(value);
    case kPropertyIDFilter:
      return ToFilterProperty(value);
    case kPropertyIDGridTemplateColumns:
    case kPropertyIDGridTemplateRows:
    case kPropertyIDGridAutoColumns:
    case kPropertyIDGridAutoRows:
      return ToGridTrackSizingProperty(value);
    case kPropertyIDVerticalAlign:
      return ToVerticalAlignProperty(value);
    case kPropertyIDXAutoFontSize:
      return ToXAutoFontSizeProperty(value);
    case kPropertyIDXAutoFontSizePresetSizes:
      return ToXAutoFontSizePresetSizesProperty(value);
    default:
      return value.AsJsonString();
  }
}

std::string CSSDecoder::ToFlexAlignType(FlexAlignType type) {
  switch (type) {
    case FlexAlignType::kAuto:
      return "auto";
    case FlexAlignType::kStretch:
      return "stretch";
    case FlexAlignType::kFlexStart:
      return "flex-start";
    case FlexAlignType::kFlexEnd:
      return "flex-end";
    case FlexAlignType::kCenter:
      return "center";
    case FlexAlignType::kBaseline:
      return "baseline";
    case FlexAlignType::kStart:
      return "start";
    case FlexAlignType::kEnd:
      return "end";
  }
}

std::string CSSDecoder::ToLengthType(lynx::starlight::LengthValueType type) {
  if (type == LengthValueType::kAuto) {
    return "auto";
  }
  LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE,
              "no such enum for type: %d", (int)type);
  return "";
}

std::string CSSDecoder::ToOverflowType(lynx::starlight::OverflowType type) {
  switch (type) {
    case OverflowType::kVisible:
      return "visible";
    case OverflowType::kHidden:
      return "hidden";
    case OverflowType::kScroll:
      return "scroll";
  }
}

std::string CSSDecoder::ToTimingFunctionType(
    lynx::starlight::TimingFunctionType type) {
  switch (type) {
    case TimingFunctionType::kLinear:
      return "linear";
    case TimingFunctionType::kEaseIn:
      return "ease-in";
    case TimingFunctionType::kEaseOut:
      return "ease-out";
    case TimingFunctionType::kEaseInEaseOut:
      return "ease-in-out";
    case TimingFunctionType::kSquareBezier:
      return "square-bezier";
    case TimingFunctionType::kCubicBezier:
      return "cubic-bezier";
    default:
      LynxWarning(false, error::E_CSS_UNSUPPORTED_VALUE,
                  "%d timing function type is invalid.", (int)type);
      return "";
  }
}

std::string CSSDecoder::ToAnimationPropertyType(
    lynx::starlight::AnimationPropertyType type) {
  switch (type) {
    case AnimationPropertyType::kNone:
      return "none";
    case AnimationPropertyType::kOpacity:
      return "opacity";
    case AnimationPropertyType::kScaleX:
      return "scaleX";
    case AnimationPropertyType::kScaleY:
      return "scaleY";
    case AnimationPropertyType::kScaleXY:
      return "scaleXY";
    case AnimationPropertyType::kWidth:
      return "width";
    case AnimationPropertyType::kHeight:
      return "height";
    case AnimationPropertyType::kBackgroundColor:
      return "background-color";
    case AnimationPropertyType::kColor:
      return "color";
    case AnimationPropertyType::kVisibility:
      return "visibility";
    case AnimationPropertyType::kLeft:
      return "left";
    case AnimationPropertyType::kTop:
      return "top";
    case AnimationPropertyType::kRight:
      return "right";
    case AnimationPropertyType::kBottom:
      return "bottom";
    case AnimationPropertyType::kTransform:
      return "transform";
    case AnimationPropertyType::kAll:
    case AnimationPropertyType::kLegacyAll_3:
    case AnimationPropertyType::kLegacyAll_2:
    case AnimationPropertyType::kLegacyAll_1:
      return "all";
    case starlight::AnimationPropertyType::kMaxWidth:
      return "max-width";
    case starlight::AnimationPropertyType::kMinWidth:
      return "min-width";
    case starlight::AnimationPropertyType::kMaxHeight:
      return "max-height";
    case starlight::AnimationPropertyType::kMinHeight:
      return "min-height";
    case starlight::AnimationPropertyType::kMarginLeft:
      return "margin-left";
    case starlight::AnimationPropertyType::kMarginRight:
      return "margin-right";
    case starlight::AnimationPropertyType::kMarginTop:
      return "margin-top";
    case starlight::AnimationPropertyType::kMarginBottom:
      return "margin-bottom";
    case starlight::AnimationPropertyType::kPaddingLeft:
      return "padding-left";
    case starlight::AnimationPropertyType::kPaddingRight:
      return "padding-right";
    case starlight::AnimationPropertyType::kPaddingTop:
      return "padding-top";
    case starlight::AnimationPropertyType::kPaddingBottom:
      return "padding-bottom";
    case starlight::AnimationPropertyType::kBorderLeftWidth:
      return "border-left-width";
    case starlight::AnimationPropertyType::kBorderLeftColor:
      return "border-left-color";
    case starlight::AnimationPropertyType::kBorderRightWidth:
      return "border-right-width";
    case starlight::AnimationPropertyType::kBorderRightColor:
      return "border-right-color";
    case starlight::AnimationPropertyType::kBorderTopWidth:
      return "border-top-width";
    case starlight::AnimationPropertyType::kBorderTopColor:
      return "border-top-color";
    case starlight::AnimationPropertyType::kBorderBottomWidth:
      return "border-bottom-width";
    case starlight::AnimationPropertyType::kBorderBottomColor:
      return "border-bottom-color";
    case starlight::AnimationPropertyType::kFlexBasis:
      return "flex-basis";
    case starlight::AnimationPropertyType::kFlexGrow:
      return "flex-grow";
    case starlight::AnimationPropertyType::kBorderWidth:
      return "border-width";
    case starlight::AnimationPropertyType::kBorderColor:
      return "border-color";
    case starlight::AnimationPropertyType::kMargin:
      return "margin";
    case starlight::AnimationPropertyType::kPadding:
      return "padding";
    case starlight::AnimationPropertyType::kFilter:
      return "filter";
    case starlight::AnimationPropertyType::kBoxShadow:
      return "box-shadow";
  }
}

std::string CSSDecoder::ToBorderStyleType(
    lynx::starlight::BorderStyleType type) {
  switch (type) {
    case BorderStyleType::kSolid:
      return "solid";
    case BorderStyleType::kDashed:
      return "dashed";
    case BorderStyleType::kDotted:
      return "dotted";
    case BorderStyleType::kDouble:
      return "double";
    case BorderStyleType::kGroove:
      return "groove";
    case BorderStyleType::kRidge:
      return "ridge";
    case BorderStyleType::kInset:
      return "inset";
    case BorderStyleType::kOutset:
      return "outset";
    case BorderStyleType::kHide:
      return "hide";
    case BorderStyleType::kNone:
      return "none";
    case BorderStyleType::kUndefined:
      return "undefined";
  }
}

std::string CSSDecoder::ToShadowOption(lynx::starlight::ShadowOption option) {
  switch (option) {
    case ShadowOption::kNone:
      return "none";
    case ShadowOption::kInset:
      return "inset";
    case ShadowOption::kInitial:
      return "initial";
    case ShadowOption::kInherit:
      return "inherit";
  }
}

std::string CSSDecoder::ToAnimationDirectionType(AnimationDirectionType type) {
  switch (type) {
    case AnimationDirectionType::kNormal:
      return "normal";
    case AnimationDirectionType::kReverse:
      return "reverse";
    case AnimationDirectionType::kAlternate:
      return "alternate";
    case AnimationDirectionType::kAlternateReverse:
      return "alternate-reverse";
  }
}

std::string CSSDecoder::ToAnimationFillModeType(AnimationFillModeType type) {
  switch (type) {
    case AnimationFillModeType::kNone:
      return "none";
    case AnimationFillModeType::kForwards:
      return "forwards";
    case AnimationFillModeType::kBackwards:
      return "backwards";
    case AnimationFillModeType::kBoth:
      return "both";
  }
}

std::string CSSDecoder::ToAnimationPlayStateType(AnimationPlayStateType type) {
  switch (type) {
    case AnimationPlayStateType::kPaused:
      return "paused";
    case AnimationPlayStateType::kRunning:
      return "running";
  }
}

std::string CSSDecoder::ToJustifyType(JustifyType type) {
  switch (type) {
    case JustifyType::kAuto:
      return "auto";
    case JustifyType::kStretch:
      return "stretch";
    case JustifyType::kStart:
      return "start";
    case JustifyType::kEnd:
      return "end";
    case JustifyType::kCenter:
      return "center";
  }
}

std::string CSSDecoder::ToGridAutoFlowType(GridAutoFlowType type) {
  switch (type) {
    case GridAutoFlowType::kRow:
      return "row";
    case GridAutoFlowType::kColumn:
      return "column";
    case GridAutoFlowType::kDense:
      return "dense";
    case GridAutoFlowType::kRowDense:
      return "row dense";
    case GridAutoFlowType::kColumnDense:
      return "column dense";
  }
}

std::string CSSDecoder::ToRgbaFromColorValue(const std::string &color_value) {
  lynx::tasm::CSSColor color;
  std::string real_color_value;
  if (lynx::tasm::CSSColor::Parse(color_value, color)) {
    real_color_value = std::to_string(color.Cast());
  } else {
    real_color_value = color_value;
  }

  std::stringstream ss(real_color_value);
  unsigned int color_val;
  ss >> color_val;
  color.a_ = (color_val & 0xff000000) >> 24;
  color.r_ = (color_val & 0x00ff0000) >> 16;
  color.g_ = (color_val & 0x0000ff00) >> 8;
  color.b_ = color_val & 0x000000ff;

  std::stringstream color_string;
  if ((static_cast<unsigned int>(color.a_)) != 255) {
    color_string << "rgba(" << static_cast<unsigned int>(color.r_) << ","
                 << static_cast<unsigned int>(color.g_) << ","
                 << static_cast<unsigned int>(color.b_) << "," << color.a_ / 255
                 << ")";
  } else {
    color_string << "rgb(" << static_cast<unsigned int>(color.r_) << ","
                 << static_cast<unsigned int>(color.g_) << ","
                 << static_cast<unsigned int>(color.b_) << ")";
  }
  return color_string.str();
}

std::string CSSDecoder::ToRgbaFromRgbaValue(const std::string &r,
                                            const std::string &g,
                                            const std::string &b,
                                            const std::string &a) {
  std::stringstream color_string;
  color_string << "rgba(" << r << "," << g << "," << b << "," << a.substr(0, 5)
               << ")";
  return color_string.str();
}

std::string CSSDecoder::ToPxValue(double px_value) {
  std::ostringstream oss;
  // remove last zero && add px suffix
  oss << px_value << "px";
  return oss.str();
}

}  // namespace tasm
}  // namespace lynx
