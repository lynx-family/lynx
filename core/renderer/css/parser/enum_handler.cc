// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/enum_handler.h"

#include <string>
#include <string_view>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {

namespace {
// AUTO INSERT, DON'T CHANGE IT!
using starlight::PositionType;
static bool ToPositionType(std::string_view str, int& result) {
  PositionType type = PositionType::kRelative;
  if (str == "absolute") {
    type = PositionType::kAbsolute;
  } else if (str == "relative") {
    type = PositionType::kRelative;
  } else if (str == "fixed") {
    type = PositionType::kFixed;
  } else if (str == "sticky") {
    type = PositionType::kSticky;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::BoxSizingType;
static bool ToBoxSizingType(std::string_view str, int& result) {
  BoxSizingType type = BoxSizingType::kAuto;
  if (str == "border-box") {
    type = BoxSizingType::kBorderBox;
  } else if (str == "content-box") {
    type = BoxSizingType::kContentBox;
  } else if (str == "auto") {
    type = BoxSizingType::kAuto;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::DisplayType;
static bool ToDisplayType(std::string_view str, int& result) {
  DisplayType type = DisplayType::kAuto;
  if (str == "none") {
    type = DisplayType::kNone;
  } else if (str == "flex") {
    type = DisplayType::kFlex;
  } else if (str == "grid") {
    type = DisplayType::kGrid;
  } else if (str == "linear") {
    type = DisplayType::kLinear;
  } else if (str == "relative") {
    type = DisplayType::kRelative;
  } else if (str == "block") {
    type = DisplayType::kBlock;
  } else if (str == "auto") {
    type = DisplayType::kAuto;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::WhiteSpaceType;
static bool ToWhiteSpaceType(std::string_view str, int& result) {
  WhiteSpaceType type = WhiteSpaceType::kNormal;
  if (str == "normal") {
    type = WhiteSpaceType::kNormal;
  } else if (str == "nowrap") {
    type = WhiteSpaceType::kNowrap;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::TextAlignType;
static bool ToTextAlignType(std::string_view str, int& result) {
  TextAlignType type = TextAlignType::kStart;
  if (str == "left") {
    type = TextAlignType::kLeft;
  } else if (str == "center") {
    type = TextAlignType::kCenter;
  } else if (str == "right") {
    type = TextAlignType::kRight;
  } else if (str == "start") {
    type = TextAlignType::kStart;
  } else if (str == "end") {
    type = TextAlignType::kEnd;
  } else if (str == "justify") {
    type = TextAlignType::kJustify;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::TextOverflowType;
static bool ToTextOverflowType(std::string_view str, int& result) {
  TextOverflowType type = TextOverflowType::kClip;
  if (str == "clip") {
    type = TextOverflowType::kClip;
  } else if (str == "ellipsis") {
    type = TextOverflowType::kEllipsis;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::FontWeightType;
static bool ToFontWeightType(std::string_view str, int& result) {
  FontWeightType type = FontWeightType::kNormal;
  if (str == "normal") {
    type = FontWeightType::kNormal;
  } else if (str == "bold") {
    type = FontWeightType::kBold;
  } else if (str == "100") {
    type = FontWeightType::k100;
  } else if (str == "200") {
    type = FontWeightType::k200;
  } else if (str == "300") {
    type = FontWeightType::k300;
  } else if (str == "400") {
    type = FontWeightType::k400;
  } else if (str == "500") {
    type = FontWeightType::k500;
  } else if (str == "600") {
    type = FontWeightType::k600;
  } else if (str == "700") {
    type = FontWeightType::k700;
  } else if (str == "800") {
    type = FontWeightType::k800;
  } else if (str == "900") {
    type = FontWeightType::k900;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::FlexDirectionType;
static bool ToFlexDirectionType(std::string_view str, int& result) {
  FlexDirectionType type = FlexDirectionType::kRow;
  if (str == "column") {
    type = FlexDirectionType::kColumn;
  } else if (str == "row") {
    type = FlexDirectionType::kRow;
  } else if (str == "row-reverse") {
    type = FlexDirectionType::kRowReverse;
  } else if (str == "column-reverse") {
    type = FlexDirectionType::kColumnReverse;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::FlexWrapType;
static bool ToFlexWrapType(std::string_view str, int& result) {
  FlexWrapType type = FlexWrapType::kNowrap;
  if (str == "wrap") {
    type = FlexWrapType::kWrap;
  } else if (str == "nowrap") {
    type = FlexWrapType::kNowrap;
  } else if (str == "wrap-reverse") {
    type = FlexWrapType::kWrapReverse;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::AlignContentType;
static bool ToAlignContentType(std::string_view str, int& result) {
  AlignContentType type = AlignContentType::kStretch;
  if (str == "flex-start") {
    type = AlignContentType::kFlexStart;
  } else if (str == "flex-end") {
    type = AlignContentType::kFlexEnd;
  } else if (str == "center") {
    type = AlignContentType::kCenter;
  } else if (str == "stretch") {
    type = AlignContentType::kStretch;
  } else if (str == "space-between") {
    type = AlignContentType::kSpaceBetween;
  } else if (str == "space-around") {
    type = AlignContentType::kSpaceAround;
  } else if (str == "start") {
    type = AlignContentType::kFlexStart;
  } else if (str == "end") {
    type = AlignContentType::kFlexEnd;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::JustifyContentType;
static bool ToJustifyContentType(std::string_view str, int& result) {
  JustifyContentType type = JustifyContentType::kStretch;
  if (str == "flex-start") {
    type = JustifyContentType::kFlexStart;
  } else if (str == "center") {
    type = JustifyContentType::kCenter;
  } else if (str == "flex-end") {
    type = JustifyContentType::kFlexEnd;
  } else if (str == "space-between") {
    type = JustifyContentType::kSpaceBetween;
  } else if (str == "space-around") {
    type = JustifyContentType::kSpaceAround;
  } else if (str == "space-evenly") {
    type = JustifyContentType::kSpaceEvenly;
  } else if (str == "stretch") {
    type = JustifyContentType::kStretch;
  } else if (str == "start") {
    type = JustifyContentType::kFlexStart;
  } else if (str == "end") {
    type = JustifyContentType::kFlexEnd;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::FontStyleType;
static bool ToFontStyleType(std::string_view str, int& result) {
  FontStyleType type = FontStyleType::kNormal;
  if (str == "normal") {
    type = FontStyleType::kNormal;
  } else if (str == "italic") {
    type = FontStyleType::kItalic;
  } else if (str == "oblique") {
    type = FontStyleType::kOblique;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::LinearOrientationType;
static bool ToLinearOrientationType(std::string_view str, int& result) {
  LinearOrientationType type = LinearOrientationType::kVertical;
  if (str == "horizontal") {
    type = LinearOrientationType::kHorizontal;
  } else if (str == "vertical") {
    type = LinearOrientationType::kVertical;
  } else if (str == "horizontal-reverse") {
    type = LinearOrientationType::kHorizontalReverse;
  } else if (str == "vertical-reverse") {
    type = LinearOrientationType::kVerticalReverse;
  } else if (str == "row") {
    type = LinearOrientationType::kRow;
  } else if (str == "column") {
    type = LinearOrientationType::kColumn;
  } else if (str == "row-reverse") {
    type = LinearOrientationType::kRowReverse;
  } else if (str == "column-reverse") {
    type = LinearOrientationType::kColumnReverse;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::LinearGravityType;
static bool ToLinearGravityType(std::string_view str, int& result) {
  LinearGravityType type = LinearGravityType::kNone;
  if (str == "none") {
    type = LinearGravityType::kNone;
  } else if (str == "top") {
    type = LinearGravityType::kTop;
  } else if (str == "bottom") {
    type = LinearGravityType::kBottom;
  } else if (str == "left") {
    type = LinearGravityType::kLeft;
  } else if (str == "right") {
    type = LinearGravityType::kRight;
  } else if (str == "center-vertical") {
    type = LinearGravityType::kCenterVertical;
  } else if (str == "center-horizontal") {
    type = LinearGravityType::kCenterHorizontal;
  } else if (str == "space-between") {
    type = LinearGravityType::kSpaceBetween;
  } else if (str == "start") {
    type = LinearGravityType::kStart;
  } else if (str == "end") {
    type = LinearGravityType::kEnd;
  } else if (str == "center") {
    type = LinearGravityType::kCenter;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::LinearLayoutGravityType;
static bool ToLinearLayoutGravityType(std::string_view str, int& result) {
  LinearLayoutGravityType type = LinearLayoutGravityType::kNone;
  if (str == "none") {
    type = LinearLayoutGravityType::kNone;
  } else if (str == "top") {
    type = LinearLayoutGravityType::kTop;
  } else if (str == "bottom") {
    type = LinearLayoutGravityType::kBottom;
  } else if (str == "left") {
    type = LinearLayoutGravityType::kLeft;
  } else if (str == "right") {
    type = LinearLayoutGravityType::kRight;
  } else if (str == "center-vertical") {
    type = LinearLayoutGravityType::kCenterVertical;
  } else if (str == "center-horizontal") {
    type = LinearLayoutGravityType::kCenterHorizontal;
  } else if (str == "fill-vertical") {
    type = LinearLayoutGravityType::kFillVertical;
  } else if (str == "fill-horizontal") {
    type = LinearLayoutGravityType::kFillHorizontal;
  } else if (str == "center") {
    type = LinearLayoutGravityType::kCenter;
  } else if (str == "stretch") {
    type = LinearLayoutGravityType::kStretch;
  } else if (str == "start") {
    type = LinearLayoutGravityType::kStart;
  } else if (str == "end") {
    type = LinearLayoutGravityType::kEnd;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::VisibilityType;
static bool ToVisibilityType(std::string_view str, int& result) {
  VisibilityType type = VisibilityType::kVisible;
  if (str == "hidden") {
    type = VisibilityType::kHidden;
  } else if (str == "visible") {
    type = VisibilityType::kVisible;
  } else if (str == "none") {
    type = VisibilityType::kNone;
  } else if (str == "collapse") {
    type = VisibilityType::kCollapse;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::WordBreakType;
static bool ToWordBreakType(std::string_view str, int& result) {
  WordBreakType type = WordBreakType::kNormal;
  if (str == "normal") {
    type = WordBreakType::kNormal;
  } else if (str == "break-all") {
    type = WordBreakType::kBreakAll;
  } else if (str == "keep-all") {
    type = WordBreakType::kKeepAll;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::DirectionType;
static bool ToDirectionType(std::string_view str, int& result) {
  DirectionType type = DirectionType::kNormal;
  if (str == "normal") {
    type = DirectionType::kNormal;
  } else if (str == "lynx-rtl") {
    type = DirectionType::kLynxRtl;
  } else if (str == "rtl") {
    type = DirectionType::kRtl;
  } else if (str == "ltr") {
    type = DirectionType::kLtr;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::RelativeCenterType;
static bool ToRelativeCenterType(std::string_view str, int& result) {
  RelativeCenterType type = RelativeCenterType::kNone;
  if (str == "none") {
    type = RelativeCenterType::kNone;
  } else if (str == "vertical") {
    type = RelativeCenterType::kVertical;
  } else if (str == "horizontal") {
    type = RelativeCenterType::kHorizontal;
  } else if (str == "both") {
    type = RelativeCenterType::kBoth;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::LinearCrossGravityType;
static bool ToLinearCrossGravityType(std::string_view str, int& result) {
  LinearCrossGravityType type = LinearCrossGravityType::kNone;
  if (str == "none") {
    type = LinearCrossGravityType::kNone;
  } else if (str == "start") {
    type = LinearCrossGravityType::kStart;
  } else if (str == "end") {
    type = LinearCrossGravityType::kEnd;
  } else if (str == "center") {
    type = LinearCrossGravityType::kCenter;
  } else if (str == "stretch") {
    type = LinearCrossGravityType::kStretch;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::ImageRenderingType;
static bool ToImageRenderingType(std::string_view str, int& result) {
  ImageRenderingType type = ImageRenderingType::kAuto;
  if (str == "auto") {
    type = ImageRenderingType::kAuto;
  } else if (str == "crisp-edges") {
    type = ImageRenderingType::kCrispEdges;
  } else if (str == "pixelated") {
    type = ImageRenderingType::kPixelated;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::HyphensType;
static bool ToHyphensType(std::string_view str, int& result) {
  HyphensType type = HyphensType::kManual;
  if (str == "none") {
    type = HyphensType::kNone;
  } else if (str == "manual") {
    type = HyphensType::kManual;
  } else if (str == "auto") {
    type = HyphensType::kAuto;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::XAppRegionType;
static bool ToXAppRegionType(std::string_view str, int& result) {
  XAppRegionType type = XAppRegionType::kNone;
  if (str == "none") {
    type = XAppRegionType::kNone;
  } else if (str == "drag") {
    type = XAppRegionType::kDrag;
  } else if (str == "no-drag") {
    type = XAppRegionType::kNoDrag;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::XAnimationColorInterpolationType;
static bool ToXAnimationColorInterpolationType(std::string_view str,
                                               int& result) {
  XAnimationColorInterpolationType type =
      XAnimationColorInterpolationType::kAuto;
  if (str == "auto") {
    type = XAnimationColorInterpolationType::kAuto;
  } else if (str == "sRGB") {
    type = XAnimationColorInterpolationType::kSRGB;
  } else if (str == "linearRGB") {
    type = XAnimationColorInterpolationType::kLinearRGB;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

// AUTO INSERT END, DON'T CHANGE IT!

using starlight::FlexAlignType;
static bool ToFlexAlignType(CSSPropertyID key, std::string_view str,
                            int& result) {
  FlexAlignType type = FlexAlignType::kAuto;
  if (str == "flex-start") {
    type = FlexAlignType::kFlexStart;
  } else if (str == "flex-end") {
    type = FlexAlignType::kFlexEnd;
  } else if (str == "center") {
    type = FlexAlignType::kCenter;
  } else if (str == "stretch") {
    type = FlexAlignType::kStretch;
  } else if (str == "baseline") {
    type = FlexAlignType::kBaseline;
  } else if (str == "auto") {
    type = FlexAlignType::kAuto;
    // Compatible with the old version
    if (key == kPropertyIDAlignItems) {
      type = FlexAlignType::kStretch;
    }
  } else if (str == "start") {
    type = FlexAlignType::kStart;
  } else if (str == "end") {
    type = FlexAlignType::kEnd;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::OverflowType;
static bool ToOverflowType(std::string_view str, int& result) {
  OverflowType type = OverflowType::kHidden;
  if (str == "visible") {
    type = OverflowType::kVisible;
  } else if (str == "scroll") {
    type = OverflowType::kScroll;
  } else if (str == "hidden") {
    type = OverflowType::kHidden;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::JustifyType;
static bool ToJustifyType(CSSPropertyID key, std::string_view str,
                          int& result) {
  JustifyType type = JustifyType::kAuto;
  if (str == "start") {
    type = JustifyType::kStart;
  } else if (str == "end") {
    type = JustifyType::kEnd;
  } else if (str == "center") {
    type = JustifyType::kCenter;
  } else if (str == "stretch") {
    type = JustifyType::kStretch;
  } else if (str == "auto") {
    type = JustifyType::kAuto;
  } else {
    return false;
  }
  result = static_cast<int>(type);
  return true;
}

using starlight::GridAutoFlowType;
static bool ToGridAutoFlowType(std::string_view str, int& result) {
  GridAutoFlowType type = GridAutoFlowType::kRow;

  bool has_row = str.find("row") != std::string::npos;
  bool has_dense = str.find("dense") != std::string::npos;
  bool has_column = str.find("column") != std::string::npos;
  if (has_row && has_column) {
    return false;
  }
  if (!has_row && !has_column && !has_dense) {
    return false;
  }

  if (has_dense) {
    if (has_row) {
      type = GridAutoFlowType::kRowDense;
    } else if (has_column) {
      type = GridAutoFlowType::kColumnDense;
    } else {
      type = GridAutoFlowType::kDense;
    }
  } else {
    if (has_row) {
      type = GridAutoFlowType::kRow;
    } else if (has_column) {
      type = GridAutoFlowType::kColumn;
    }
  }
  result = static_cast<int>(type);
  return true;
}

}  // namespace
namespace EnumHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  auto str = input.StringView();
  int result(0);
  bool success = false;
  switch (key) {
    // AUTO INSERT, DON'T CHANGE IT!
    case kPropertyIDPosition:
      success = ToPositionType(str, result);
      break;
    case kPropertyIDBoxSizing:
      success = ToBoxSizingType(str, result);
      break;
    case kPropertyIDDisplay:
      success = ToDisplayType(str, result);
      break;
    case kPropertyIDWhiteSpace:
      success = ToWhiteSpaceType(str, result);
      break;
    case kPropertyIDTextAlign:
      success = ToTextAlignType(str, result);
      break;
    case kPropertyIDTextOverflow:
      success = ToTextOverflowType(str, result);
      break;
    case kPropertyIDFontWeight:
      success = ToFontWeightType(str, result);
      break;
    case kPropertyIDFlexDirection:
      success = ToFlexDirectionType(str, result);
      break;
    case kPropertyIDFlexWrap:
      success = ToFlexWrapType(str, result);
      break;
    case kPropertyIDAlignContent:
      success = ToAlignContentType(str, result);
      break;
    case kPropertyIDJustifyContent:
      success = ToJustifyContentType(str, result);
      break;
    case kPropertyIDFontStyle:
      success = ToFontStyleType(str, result);
      break;
    case kPropertyIDLinearOrientation:
      success = ToLinearOrientationType(str, result);
      break;
    case kPropertyIDLinearGravity:
      success = ToLinearGravityType(str, result);
      break;
    case kPropertyIDLinearLayoutGravity:
      success = ToLinearLayoutGravityType(str, result);
      break;
    case kPropertyIDVisibility:
      success = ToVisibilityType(str, result);
      break;
    case kPropertyIDWordBreak:
      success = ToWordBreakType(str, result);
      break;
    case kPropertyIDDirection:
      success = ToDirectionType(str, result);
      break;
    case kPropertyIDRelativeCenter:
      success = ToRelativeCenterType(str, result);
      break;
    case kPropertyIDLinearCrossGravity:
      success = ToLinearCrossGravityType(str, result);
      break;
    case kPropertyIDImageRendering:
      success = ToImageRenderingType(str, result);
      break;
    case kPropertyIDHyphens:
      success = ToHyphensType(str, result);
      break;
    case kPropertyIDXAppRegion:
      success = ToXAppRegionType(str, result);
      break;
    case kPropertyIDXAnimationColorInterpolation:
      success = ToXAnimationColorInterpolationType(str, result);
      break;
    // AUTO INSERT END, DON'T CHANGE IT!
    case kPropertyIDLinearDirection:
      success = ToLinearOrientationType(str, result);
      break;
    case kPropertyIDAlignItems:
    case kPropertyIDAlignSelf:
      success = ToFlexAlignType(key, str, result);
      break;
    case kPropertyIDOverflow:
    case kPropertyIDOverflowX:
    case kPropertyIDOverflowY:
      success = ToOverflowType(str, result);
      break;
    case kPropertyIDJustifyItems:
    case kPropertyIDJustifySelf:
      success = ToJustifyType(key, str, result);
      break;
    case kPropertyIDGridAutoFlow:
      success = ToGridAutoFlowType(str, result);
      break;
    default:
      break;
  }

  CSS_HANDLER_FAIL_IF_NOT(success, configs.enable_css_strict_mode,
                          TYPE_UNSUPPORTED,
                          CSSProperty::GetPropertyNameCStr(key), str.data())
  output.emplace_or_assign(key, result, CSSValue::kCreateEnumTag);
  return true;
}

HANDLER_REGISTER_IMPL() {
  // AUTO INSERT, DON'T CHANGE IT!
  array[kPropertyIDPosition] = &Handle;
  array[kPropertyIDBoxSizing] = &Handle;
  array[kPropertyIDDisplay] = &Handle;
  array[kPropertyIDWhiteSpace] = &Handle;
  array[kPropertyIDTextAlign] = &Handle;
  array[kPropertyIDTextOverflow] = &Handle;
  array[kPropertyIDFontWeight] = &Handle;
  array[kPropertyIDFlexDirection] = &Handle;
  array[kPropertyIDFlexWrap] = &Handle;
  array[kPropertyIDAlignContent] = &Handle;
  array[kPropertyIDJustifyContent] = &Handle;
  array[kPropertyIDFontStyle] = &Handle;
  array[kPropertyIDLinearOrientation] = &Handle;
  array[kPropertyIDLinearGravity] = &Handle;
  array[kPropertyIDLinearLayoutGravity] = &Handle;
  array[kPropertyIDVisibility] = &Handle;
  array[kPropertyIDWordBreak] = &Handle;
  array[kPropertyIDDirection] = &Handle;
  array[kPropertyIDRelativeCenter] = &Handle;
  array[kPropertyIDLinearCrossGravity] = &Handle;
  array[kPropertyIDImageRendering] = &Handle;
  array[kPropertyIDHyphens] = &Handle;
  array[kPropertyIDXAppRegion] = &Handle;
  array[kPropertyIDXAnimationColorInterpolation] = &Handle;
  // AUTO INSERT END, DON'T CHANGE IT!
  array[kPropertyIDLinearDirection] = &Handle;
  array[kPropertyIDAlignItems] = &Handle;
  array[kPropertyIDAlignSelf] = &Handle;
  array[kPropertyIDOverflow] = &Handle;
  array[kPropertyIDOverflowX] = &Handle;
  array[kPropertyIDOverflowY] = &Handle;
  array[kPropertyIDJustifyItems] = &Handle;
  array[kPropertyIDJustifySelf] = &Handle;
  array[kPropertyIDGridAutoFlow] = &Handle;
}

}  // namespace EnumHandler
}  // namespace tasm
}  // namespace lynx
