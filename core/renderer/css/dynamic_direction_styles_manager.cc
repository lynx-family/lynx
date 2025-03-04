// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/dynamic_direction_styles_manager.h"

#include <unordered_map>

#include "base/include/no_destructor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace tasm {
using starlight::DirectionType;
using starlight::TextAlignType;

namespace {
// For logic style
const std::unordered_map<CSSPropertyID, CSSPropertyID>& GetLogicStyleMapping() {
  static base::NoDestructor<std::unordered_map<CSSPropertyID, CSSPropertyID>>
      kDefaultDirectionAwareProps{{
          {kPropertyIDMarginInlineStart, kPropertyIDMarginLeft},
          {kPropertyIDMarginInlineEnd, kPropertyIDMarginRight},
          {kPropertyIDPaddingInlineStart, kPropertyIDPaddingLeft},
          {kPropertyIDPaddingInlineEnd, kPropertyIDPaddingRight},
          {kPropertyIDBorderInlineStartWidth, kPropertyIDBorderLeftWidth},
          {kPropertyIDBorderInlineEndWidth, kPropertyIDBorderRightWidth},
          {kPropertyIDBorderInlineStartStyle, kPropertyIDBorderLeftStyle},
          {kPropertyIDBorderInlineEndStyle, kPropertyIDBorderRightStyle},
          {kPropertyIDBorderInlineStartColor, kPropertyIDBorderLeftColor},
          {kPropertyIDBorderInlineEndColor, kPropertyIDBorderRightColor},
          {kPropertyIDBorderStartStartRadius, kPropertyIDBorderTopLeftRadius},
          {kPropertyIDBorderStartEndRadius, kPropertyIDBorderTopRightRadius},
          {kPropertyIDBorderEndStartRadius, kPropertyIDBorderBottomLeftRadius},
          {kPropertyIDBorderEndEndRadius, kPropertyIDBorderBottomRightRadius},
          {kPropertyIDRelativeAlignInlineStart, kPropertyIDRelativeAlignLeft},
          {kPropertyIDRelativeAlignInlineEnd, kPropertyIDRelativeAlignRight},
          {kPropertyIDRelativeInlineStartOf, kPropertyIDRelativeLeftOf},
          {kPropertyIDRelativeInlineEndOf, kPropertyIDRelativeRightOf},
          {kPropertyIDInsetInlineStart, kPropertyIDLeft},
          {kPropertyIDInsetInlineEnd, kPropertyIDRight},
      }};
  return *kDefaultDirectionAwareProps;
}

// For lynx-rtl. first ==rtl==> second
const std::unordered_map<CSSPropertyID, CSSPropertyID>&
GetRTLDirectionMapping() {
  static base::NoDestructor<std::unordered_map<CSSPropertyID, CSSPropertyID>>
      kDefaultDirectionAwareProps{{
          {kPropertyIDLeft, kPropertyIDRight},
          {kPropertyIDRight, kPropertyIDLeft},
          {kPropertyIDMarginLeft, kPropertyIDMarginRight},
          {kPropertyIDMarginRight, kPropertyIDMarginLeft},
          {kPropertyIDPaddingLeft, kPropertyIDPaddingRight},
          {kPropertyIDPaddingRight, kPropertyIDPaddingLeft},
          {kPropertyIDBorderLeftWidth, kPropertyIDBorderRightWidth},
          {kPropertyIDBorderRightWidth, kPropertyIDBorderLeftWidth},
          {kPropertyIDBorderLeftStyle, kPropertyIDBorderRightStyle},
          {kPropertyIDBorderRightStyle, kPropertyIDBorderLeftStyle},
          {kPropertyIDBorderLeftColor, kPropertyIDBorderRightColor},
          {kPropertyIDBorderRightColor, kPropertyIDBorderLeftColor},
          {kPropertyIDBorderTopLeftRadius, kPropertyIDBorderTopRightRadius},
          {kPropertyIDBorderTopRightRadius, kPropertyIDBorderTopLeftRadius},
          {kPropertyIDBorderBottomLeftRadius,
           kPropertyIDBorderBottomRightRadius},
          {kPropertyIDBorderBottomRightRadius,
           kPropertyIDBorderBottomLeftRadius},
          {kPropertyIDRelativeAlignLeft, kPropertyIDRelativeAlignRight},
          {kPropertyIDRelativeAlignRight, kPropertyIDRelativeAlignLeft},
          {kPropertyIDRelativeLeftOf, kPropertyIDRelativeRightOf},
          {kPropertyIDRelativeRightOf, kPropertyIDRelativeLeftOf},
      }};
  return *kDefaultDirectionAwareProps;
}
}  // namespace

// static
bool IsLogicalDirectionStyle(CSSPropertyID css_id) {
  return css_id == kPropertyIDTextAlign || GetLogicStyleMapping().count(css_id);
}

// static
bool IsDirectionAwareStyle(CSSPropertyID css_id) {
  return IsLogicalDirectionStyle(css_id) ||
         GetRTLDirectionMapping().count(css_id);
}

// static
std::pair<CSSPropertyID, IsLogic> ResolveLogicStyleID(CSSPropertyID css_id) {
  CSSPropertyID trans_id = css_id;
  bool is_logic_style = false;
  // start --> left/right
  const auto& trans_map = GetLogicStyleMapping();
  const auto& trans_entry = trans_map.find(css_id);
  if (trans_entry != trans_map.end()) {
    is_logic_style = true;
    trans_id = trans_entry->second;
  }
  return {trans_id, is_logic_style};
}

// static
CSSPropertyID ResolveDirectionRelatedStyleID(CSSPropertyID trans_id,
                                             DirectionType direction,
                                             bool is_logic_style) {
  // is direction aware props
  const auto& rlt_map = GetRTLDirectionMapping();
  const auto& rlt_entry = rlt_map.find(trans_id);
  if (rlt_entry != rlt_map.end()) {
    if ((IsRTL(direction) && is_logic_style) || IsLynxRTL(direction)) {
      return rlt_entry->second;
    }
  }
  return trans_id;
}

CSSPropertyID ResolveDirectionAwareProperty(CSSPropertyID css_id,
                                            DirectionType direction) {
  auto ret_pair = ResolveLogicStyleID(css_id);
  CSSPropertyID trans_id = ret_pair.first;
  bool is_logic_style = ret_pair.second;

  if (!IsAnyRTL(direction)) {
    return trans_id;
  }

  // is direction aware props
  trans_id =
      ResolveDirectionRelatedStyleID(trans_id, direction, is_logic_style);

  return trans_id;
}

CSSStyleValue ResolveTextAlign(CSSPropertyID css_id,
                               const tasm::CSSValue& value,
                               DirectionType direction) {
  TextAlignType align_type = value.GetEnum<TextAlignType>();
  TextAlignType result_type;
  switch (align_type) {
    case TextAlignType::kStart:
      result_type = direction == DirectionType::kNormal
                        ? TextAlignType::kStart
                        : (IsAnyRTL(direction) ? TextAlignType::kRight
                                               : TextAlignType::kLeft);
      break;
    case TextAlignType::kEnd:
      result_type =
          IsAnyRTL(direction) ? TextAlignType::kLeft : TextAlignType::kRight;
      break;
    case TextAlignType::kLeft:
      result_type =
          IsLynxRTL(direction) ? TextAlignType::kRight : TextAlignType::kLeft;
      break;
    case TextAlignType::kRight:
      result_type =
          IsLynxRTL(direction) ? TextAlignType::kLeft : TextAlignType::kRight;
      break;
    default:
      result_type = align_type;
      break;
  }
  return CSSStyleValue(css_id,
                       CSSValue(lepus::Value(static_cast<int32_t>(result_type)),
                                CSSValuePattern::ENUM));
}

}  // namespace tasm
}  // namespace lynx
