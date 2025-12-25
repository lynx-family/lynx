// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/dynamic_css_styles_manager.h"

#include <memory>
#include <string>
#include <unordered_set>

#include "base/include/no_destructor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/layout_property.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
namespace lynx {
namespace tasm {
using starlight::DirectionType;
using starlight::TextAlignType;

namespace {

// Not that kPropertyIDFontSize, kPropertyIDLineSpacing,
// kPropertyIDLetterSpacing, kPropertyIDLineHeight are not simple inheritable
// props, should inherit computed style value for them.
const std::unordered_set<CSSPropertyID>& GetDefaultInheritableProps() {
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kDefaultInheritableProps(std::unordered_set<CSSPropertyID>{
          kPropertyIDFontSize, kPropertyIDFontFamily, kPropertyIDTextAlign,
          kPropertyIDLineSpacing, kPropertyIDLetterSpacing,
          kPropertyIDLineHeight, kPropertyIDFontStyle, kPropertyIDFontWeight,
          kPropertyIDColor, kPropertyIDTextDecoration, kPropertyIDTextShadow,
          kPropertyIDDirection, kPropertyIDCursor});
  return *kDefaultInheritableProps;
}

const std::unordered_set<CSSPropertyID>& GetComplexDynamicProps() {
  const static base::NoDestructor<std::unordered_set<CSSPropertyID>>
      kComplexDynamicProps(std::unordered_set<CSSPropertyID>{
          kPropertyIDTransformOrigin,
          kPropertyIDBackgroundSize,
          kPropertyIDBackgroundPosition,
          kPropertyIDBorderRadius,
          kPropertyIDBorderTopLeftRadius,
          kPropertyIDBorderTopRightRadius,
          kPropertyIDBorderBottomLeftRadius,
          kPropertyIDBorderBottomRightRadius,
          kPropertyIDBorderStartStartRadius,
          kPropertyIDBorderStartEndRadius,
          kPropertyIDBorderEndEndRadius,
          kPropertyIDBorderEndStartRadius,
          kPropertyIDTransform,
          kPropertyIDBoxShadow,
          kPropertyIDTextShadow,
          kPropertyIDGridAutoRows,
          kPropertyIDGridAutoColumns,
          kPropertyIDGridTemplateRows,
          kPropertyIDGridTemplateColumns,
      });
  return *kComplexDynamicProps;
}

inline DynamicCSSStylesManager::StyleUpdateFlags GetPercentDependency(
    CSSPropertyID id) {
  // Currently, only line-height and font size are supported to have behavior of
  // percentage unit
  if (id == kPropertyIDLineHeight) {
    return DynamicCSSStylesManager::kUpdateEm;
  }
  return DynamicCSSStylesManager::kNoUpdate;
}

inline DynamicCSSStylesManager::StyleUpdateFlags CheckFontScaleRelevance(
    CSSPropertyID id) {
  // Currently, only line-height and font size are supported to have behavior of
  // percentage unit
  if (id == kPropertyIDFontSize || id == kPropertyIDLineHeight ||
      id == kPropertyIDLetterSpacing) {
    return DynamicCSSStylesManager::kUpdateFontScale;
  }
  return DynamicCSSStylesManager::kNoUpdate;
}

}  // namespace

const std::unordered_set<CSSPropertyID>&
DynamicCSSStylesManager::GetInheritableProps() {
  return GetDefaultInheritableProps();
}

// static
CSSPropertyID DynamicCSSStylesManager::ResolveDirectionAwarePropertyID(
    CSSPropertyID id, starlight::DirectionType direction) {
  return ResolveDirectionAwareProperty(id, direction);
}

// static
// TODO(zhouzhitao): unify logic with radon element, remove this overwritten
// version of UpdateDirectionAwareDefaultStyles
void DynamicCSSStylesManager::UpdateDirectionAwareDefaultStyles(
    Element* element, starlight::DirectionType direction,
    const CSSValue& text_align_value) {
  // Currently, only text-align has direction aware default property.
  auto default_align_value = text_align_value;

  if (default_align_value.IsEmpty()) {
    default_align_value = CSSValue(TextAlignType::kStart);
  }
  const auto default_align =
      ResolveTextAlign(kPropertyIDTextAlign, default_align_value, direction);
  element->SetStyleInternal(kPropertyIDTextAlign, default_align.second);
}

DynamicCSSStylesManager::StyleUpdateFlags
DynamicCSSStylesManager::GetValueFlags(CSSPropertyID id, const CSSValue& value,
                                       bool unify_vw_vh_behavior) {
  DynamicCSSStylesManager::StyleUpdateFlags flags =
      DynamicCSSStylesManager::kNoUpdate;

  switch (value.GetPattern()) {
    case CSSValuePattern::EMPTY:
    case CSSValuePattern::ENUM:
      break;
    case CSSValuePattern::RPX:
      flags = DynamicCSSStylesManager::kUpdateScreenMetrics;
    case CSSValuePattern::PX:
    case CSSValuePattern::PPX:
      flags |= CheckFontScaleRelevance(id);
      break;
    case CSSValuePattern::PERCENT:
    case CSSValuePattern::NUMBER:
      flags = GetPercentDependency(id);
      flags |= CheckFontScaleRelevance(id);
      break;
    case CSSValuePattern::REM:
      flags = DynamicCSSStylesManager::kUpdateRem;
      break;
    case CSSValuePattern::EM:
      flags = DynamicCSSStylesManager::kUpdateEm;
      break;
    case CSSValuePattern::VW:
    case CSSValuePattern::VH:
      flags = DynamicCSSStylesManager::kUpdateViewport;
      break;
    case CSSValuePattern::CALC: {
      const auto& calc_str = value.AsStdString();
      if (calc_str.find("rpx") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateScreenMetrics;
        flags |= CheckFontScaleRelevance(id);
      }
      if (calc_str.find("em") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateEm;
      }
      if (calc_str.find("rem") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateRem;
      }
      if (calc_str.find("%") != std::string::npos) {
        flags |= GetPercentDependency(id);
      }
      if (calc_str.find("px") != std::string::npos ||
          calc_str.find("ppx") != std::string::npos) {
        flags |= CheckFontScaleRelevance(id);
      }
      if (calc_str.find("vw") != std::string::npos ||
          calc_str.find("vh") != std::string::npos ||
          calc_str.find("view_width") != std::string::npos ||
          calc_str.find("view_height") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateViewport;
      }
      if (calc_str.find("sp") != std::string::npos) {
        flags |= DynamicCSSStylesManager::kUpdateFontScale;
      }
      break;
    }
    case CSSValuePattern::ENV:
      // TODO:
      break;
    case CSSValuePattern::SP:
      flags |= DynamicCSSStylesManager::kUpdateFontScale;
    default:
      // TODO: Currently always recompute complex properties, we can
      // struturelize the properties before passing it to computed style in
      // future.
      if (GetComplexDynamicProps().count(id)) {
        flags = DynamicCSSStylesManager::kUpdateScreenMetrics |
                DynamicCSSStylesManager::kUpdateEm |
                DynamicCSSStylesManager::kUpdateRem |
                DynamicCSSStylesManager::kUpdateFontScale;
        if (unify_vw_vh_behavior) {
          flags = (flags | DynamicCSSStylesManager::kUpdateViewport);
        }
      }
      break;
  }
  if (IsDirectionAwareStyle(id)) {
    flags |= DynamicCSSStylesManager::kUpdateDirectionStyle;
  }

  return flags;
}

}  // namespace tasm
}  // namespace lynx
