// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_property.h"

#include <array>
#include <mutex>
#include <set>
#include <unordered_set>

#include "base/include/no_destructor.h"
#include "core/public/prop_bundle.h"

namespace lynx {
namespace tasm {

namespace {

constexpr CSSPropertyID kMarginLonghands[] = {
    kPropertyIDMarginTop, kPropertyIDMarginRight, kPropertyIDMarginBottom,
    kPropertyIDMarginLeft};
constexpr CSSPropertyID kPaddingLonghands[] = {
    kPropertyIDPaddingTop, kPropertyIDPaddingRight, kPropertyIDPaddingBottom,
    kPropertyIDPaddingLeft};
constexpr CSSPropertyID kBorderWidthLonghands[] = {
    kPropertyIDBorderTopWidth, kPropertyIDBorderRightWidth,
    kPropertyIDBorderBottomWidth, kPropertyIDBorderLeftWidth};
constexpr CSSPropertyID kBorderColorLonghands[] = {
    kPropertyIDBorderTopColor, kPropertyIDBorderRightColor,
    kPropertyIDBorderBottomColor, kPropertyIDBorderLeftColor};
constexpr CSSPropertyID kBorderStyleLonghands[] = {
    kPropertyIDBorderTopStyle, kPropertyIDBorderRightStyle,
    kPropertyIDBorderBottomStyle, kPropertyIDBorderLeftStyle};
constexpr CSSPropertyID kBorderLonghands[] = {
    kPropertyIDBorderTopWidth,    kPropertyIDBorderRightWidth,
    kPropertyIDBorderBottomWidth, kPropertyIDBorderLeftWidth,
    kPropertyIDBorderTopColor,    kPropertyIDBorderRightColor,
    kPropertyIDBorderBottomColor, kPropertyIDBorderLeftColor,
    kPropertyIDBorderTopStyle,    kPropertyIDBorderRightStyle,
    kPropertyIDBorderBottomStyle, kPropertyIDBorderLeftStyle};
constexpr CSSPropertyID kBorderRadiusLonghands[] = {
    kPropertyIDBorderTopLeftRadius, kPropertyIDBorderTopRightRadius,
    kPropertyIDBorderBottomRightRadius, kPropertyIDBorderBottomLeftRadius};
constexpr CSSPropertyID kBorderTopLonghands[] = {kPropertyIDBorderTopWidth,
                                                 kPropertyIDBorderTopColor,
                                                 kPropertyIDBorderTopStyle};
constexpr CSSPropertyID kBorderRightLonghands[] = {kPropertyIDBorderRightWidth,
                                                   kPropertyIDBorderRightColor,
                                                   kPropertyIDBorderRightStyle};
constexpr CSSPropertyID kBorderBottomLonghands[] = {
    kPropertyIDBorderBottomWidth, kPropertyIDBorderBottomColor,
    kPropertyIDBorderBottomStyle};
constexpr CSSPropertyID kBorderLeftLonghands[] = {kPropertyIDBorderLeftWidth,
                                                  kPropertyIDBorderLeftColor,
                                                  kPropertyIDBorderLeftStyle};
constexpr CSSPropertyID kOutlineLonghands[] = {
    kPropertyIDOutlineWidth, kPropertyIDOutlineColor, kPropertyIDOutlineStyle};
constexpr CSSPropertyID kBackgroundLonghands[] = {
    kPropertyIDBackgroundColor,    kPropertyIDBackgroundImage,
    kPropertyIDBackgroundPosition, kPropertyIDBackgroundSize,
    kPropertyIDBackgroundRepeat,   kPropertyIDBackgroundOrigin,
    kPropertyIDBackgroundClip};
constexpr CSSPropertyID kFlexLonghands[] = {
    kPropertyIDFlexGrow, kPropertyIDFlexShrink, kPropertyIDFlexBasis};
constexpr CSSPropertyID kFlexFlowLonghands[] = {kPropertyIDFlexDirection,
                                                kPropertyIDFlexWrap};
constexpr CSSPropertyID kTransitionLonghands[] = {
    kPropertyIDTransitionProperty, kPropertyIDTransitionDuration,
    kPropertyIDTransitionDelay, kPropertyIDTransitionTimingFunction};
constexpr CSSPropertyID kMaskLonghands[] = {
    kPropertyIDMaskImage,  kPropertyIDMaskPosition, kPropertyIDMaskSize,
    kPropertyIDMaskRepeat, kPropertyIDMaskOrigin,   kPropertyIDMaskClip};
constexpr CSSPropertyID kAnimationLonghands[] = {
    kPropertyIDAnimationName,           kPropertyIDAnimationDuration,
    kPropertyIDAnimationDelay,          kPropertyIDAnimationTimingFunction,
    kPropertyIDAnimationIterationCount, kPropertyIDAnimationDirection,
    kPropertyIDAnimationFillMode,       kPropertyIDAnimationPlayState};

struct ExpandedLonghands {
  const CSSPropertyID* ids;
  size_t count;
};

}  // namespace

const char* GetPropertyNameCStr(CSSPropertyID id) {
  return CSSProperty::GetPropertyNameCStr(id);
}

const std::set<CSSPropertyID> inspectorFilteredProperties{
    kPropertyIDBorder,
    kPropertyIDBorderTop,
    kPropertyIDBorderRight,
    kPropertyIDBorderBottom,
    kPropertyIDBorderLeft,
    kPropertyIDMarginInlineStart,
    kPropertyIDMarginInlineEnd,
    kPropertyIDPaddingInlineStart,
    kPropertyIDPaddingInlineEnd,
    kPropertyIDBorderInlineStartWidth,
    kPropertyIDBorderInlineEndWidth,
    kPropertyIDBorderInlineStartColor,
    kPropertyIDBorderInlineEndColor,
    kPropertyIDBorderInlineStartStyle,
    kPropertyIDBorderInlineEndStyle,
    kPropertyIDBorderStartStartRadius,
    kPropertyIDBorderEndStartRadius,
    kPropertyIDBorderStartEndRadius,
    kPropertyIDBorderEndEndRadius,
    kPropertyIDFlex,
    kPropertyIDFlexFlow,
    kPropertyIDPadding,
    kPropertyIDMargin,
    kPropertyIDInsetInlineStart,
    kPropertyIDInsetInlineEnd,
    kPropertyIDBorderWidth,
    kPropertyIDBackground,
    kPropertyIDBorderColor,
    kPropertyIDBorderStyle,
    kPropertyIDOutline};

using CSSPropertyNameToIDMap =
    std::unordered_map<base::static_string::GenericCacheKey, CSSPropertyID>;

CSSPropertyID CSSProperty::GetPropertyID(
    const base::static_string::GenericCacheKey& key) {
  const static base::NoDestructor<CSSPropertyNameToIDMap> kPropertyNameMapping{{
#define DECLARE_PROPERTY_NAME(name, c, value) \
  {kPropertyName##name, kPropertyID##name},
      FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_NAME)
#undef DECLARE_PROPERTY_NAME
  }};
  auto it = kPropertyNameMapping->find(key);
  return it != kPropertyNameMapping->end() ? it->second : kPropertyEnd;
}

const base::static_string::GenericCache& CSSProperty::GetPropertyName(
    CSSPropertyID id) {
  // The reason using std::call_once is that GenericCache is not copy
  // constructible and `NoDestructor<PropertyIDMappingArray> xx{{}}`
  // won't compile
  using PropertyIDMappingArray = std::array<base::static_string::GenericCache,
                                            CSSPropertyID::kPropertyEnd + 1>;
  static PropertyIDMappingArray* kPropertyIdMapping;
  static std::once_flag prop_id_mapping_init_flag;
  std::call_once(prop_id_mapping_init_flag, []() {
    kPropertyIdMapping = new PropertyIDMappingArray{
        "",  // start
#define DECLARE_PROPERTY_ID(name, c, value) kPropertyName##name,
        FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
            ""  // end
    };
  });
  if (id >= kPropertyStart && id <= kPropertyEnd) {
    return (*kPropertyIdMapping)[id];
  }
  return (*kPropertyIdMapping)[kPropertyStart];  // Empty string
}

// TODO: Need to auto generate from css define
size_t CSSProperty::GetShorthandExpand(CSSPropertyID id) {
  using PropertyIDShorthandArray =
      std::array<uint8_t, CSSPropertyID::kPropertyEnd + 1>;
  static PropertyIDShorthandArray* kPropertyIdShorthand;
  static std::once_flag prop_id_mapping_init_flag;
  std::call_once(prop_id_mapping_init_flag, []() {
    kPropertyIdShorthand = new PropertyIDShorthandArray();
    std::memset(kPropertyIdShorthand->data(), 0,
                sizeof(uint8_t) * kPropertyIdShorthand->size());
    (*kPropertyIdShorthand)[kPropertyIDPadding] = 4;
    (*kPropertyIdShorthand)[kPropertyIDMargin] = 4;
    (*kPropertyIdShorthand)[kPropertyIDFlex] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBackground] = 8;
    (*kPropertyIdShorthand)[kPropertyIDBorder] = 12;
    (*kPropertyIdShorthand)[kPropertyIDBorderWidth] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderRadius] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderColor] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderStyle] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderRight] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBorderLeft] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBorderTop] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBorderBottom] = 3;
    (*kPropertyIdShorthand)[kPropertyIDOutline] = 3;
    (*kPropertyIdShorthand)[kPropertyIDFlexFlow] = 2;
    (*kPropertyIdShorthand)[kPropertyIDTransition] = 5;
    (*kPropertyIdShorthand)[kPropertyIDMask] = 8;
    (*kPropertyIdShorthand)[kPropertyIDAnimation] = 9;
  });
  if (id >= kPropertyStart && id <= kPropertyEnd) {
    return (*kPropertyIdShorthand)[id];
  }
  return 0;
}

// TODO: Need to auto generate from css define
const std::unordered_set<CSSPropertyID> shorthandCSSProperties{
    kPropertyIDPadding,      kPropertyIDMargin,      kPropertyIDFlex,
    kPropertyIDBackground,   kPropertyIDBorder,      kPropertyIDBorderWidth,
    kPropertyIDBorderRadius, kPropertyIDBorderColor, kPropertyIDBorderStyle,
    kPropertyIDBorderRight,  kPropertyIDBorderLeft,  kPropertyIDBorderTop,
    kPropertyIDBorderBottom, kPropertyIDOutline,     kPropertyIDFlexFlow,
    kPropertyIDTransition,   kPropertyIDMask,        kPropertyIDAnimation};

bool CSSProperty::IsShorthandProperty(CSSPropertyID id) {
  return shorthandCSSProperties.find(id) != shorthandCSSProperties.end();
}

const CSSPropertyID* CSSProperty::GetExpandedLonghands(CSSPropertyID id,
                                                       size_t* count) {
  if (count != nullptr) {
    *count = 0;
  }

  const ExpandedLonghands* expanded = nullptr;
  switch (id) {
    case kPropertyIDMargin:
      static constexpr ExpandedLonghands kMargin = {
          kMarginLonghands, std::size(kMarginLonghands)};
      expanded = &kMargin;
      break;
    case kPropertyIDPadding:
      static constexpr ExpandedLonghands kPadding = {
          kPaddingLonghands, std::size(kPaddingLonghands)};
      expanded = &kPadding;
      break;
    case kPropertyIDBorderWidth:
      static constexpr ExpandedLonghands kBorderWidth = {
          kBorderWidthLonghands, std::size(kBorderWidthLonghands)};
      expanded = &kBorderWidth;
      break;
    case kPropertyIDBorderColor:
      static constexpr ExpandedLonghands kBorderColor = {
          kBorderColorLonghands, std::size(kBorderColorLonghands)};
      expanded = &kBorderColor;
      break;
    case kPropertyIDBorderStyle:
      static constexpr ExpandedLonghands kBorderStyle = {
          kBorderStyleLonghands, std::size(kBorderStyleLonghands)};
      expanded = &kBorderStyle;
      break;
    case kPropertyIDBorder:
      static constexpr ExpandedLonghands kBorder = {
          kBorderLonghands, std::size(kBorderLonghands)};
      expanded = &kBorder;
      break;
    case kPropertyIDBorderRadius:
      static constexpr ExpandedLonghands kBorderRadius = {
          kBorderRadiusLonghands, std::size(kBorderRadiusLonghands)};
      expanded = &kBorderRadius;
      break;
    case kPropertyIDBorderTop:
      static constexpr ExpandedLonghands kBorderTop = {
          kBorderTopLonghands, std::size(kBorderTopLonghands)};
      expanded = &kBorderTop;
      break;
    case kPropertyIDBorderRight:
      static constexpr ExpandedLonghands kBorderRight = {
          kBorderRightLonghands, std::size(kBorderRightLonghands)};
      expanded = &kBorderRight;
      break;
    case kPropertyIDBorderBottom:
      static constexpr ExpandedLonghands kBorderBottom = {
          kBorderBottomLonghands, std::size(kBorderBottomLonghands)};
      expanded = &kBorderBottom;
      break;
    case kPropertyIDBorderLeft:
      static constexpr ExpandedLonghands kBorderLeft = {
          kBorderLeftLonghands, std::size(kBorderLeftLonghands)};
      expanded = &kBorderLeft;
      break;
    case kPropertyIDOutline:
      static constexpr ExpandedLonghands kOutline = {
          kOutlineLonghands, std::size(kOutlineLonghands)};
      expanded = &kOutline;
      break;
    case kPropertyIDBackground:
      static constexpr ExpandedLonghands kBackground = {
          kBackgroundLonghands, std::size(kBackgroundLonghands)};
      expanded = &kBackground;
      break;
    case kPropertyIDFlex:
      static constexpr ExpandedLonghands kFlex = {kFlexLonghands,
                                                  std::size(kFlexLonghands)};
      expanded = &kFlex;
      break;
    case kPropertyIDFlexFlow:
      static constexpr ExpandedLonghands kFlexFlow = {
          kFlexFlowLonghands, std::size(kFlexFlowLonghands)};
      expanded = &kFlexFlow;
      break;
    case kPropertyIDTransition:
      static constexpr ExpandedLonghands kTransition = {
          kTransitionLonghands, std::size(kTransitionLonghands)};
      expanded = &kTransition;
      break;
    case kPropertyIDMask:
      static constexpr ExpandedLonghands kMask = {kMaskLonghands,
                                                  std::size(kMaskLonghands)};
      expanded = &kMask;
      break;
    case kPropertyIDAnimation:
      static constexpr ExpandedLonghands kAnimation = {
          kAnimationLonghands, std::size(kAnimationLonghands)};
      expanded = &kAnimation;
      break;
    default:
      break;
  }

  if (!expanded) {
    return nullptr;
  }
  if (count != nullptr) {
    *count = expanded->count;
  }
  return expanded->ids;
}

CSSPropertyID CSSProperty::GetTimingOptionsPropertyID(
    const base::static_string::GenericCacheKey& key) {
#define DECLARE_ANIMATIONAPI_PROPERTY_NAME(name, alias) \
  static constexpr const char kAnimationAPIPropertyName##name[] = alias;
  FOREACH_ALL_ANIMATIONAPI_PROPERTY(DECLARE_ANIMATIONAPI_PROPERTY_NAME)
#undef DECLARE_ANIMATIONAPI_PROPERTY_NAME

  const static base::NoDestructor<CSSPropertyNameToIDMap>
      kAnimationPropertyNameMapping{{
#define DECLARE_PROPERTY_NAME(name, alias) \
  {kAnimationAPIPropertyName##name, kPropertyID##name},
          FOREACH_ALL_ANIMATIONAPI_PROPERTY(DECLARE_PROPERTY_NAME)
#undef DECLARE_PROPERTY_NAME
      }};
  auto it = kAnimationPropertyNameMapping->find(key);
  return it != kAnimationPropertyNameMapping->end() ? it->second : kPropertyEnd;
}

const std::unordered_map<std::string, std::string>&
CSSProperty::GetComputeStyleMap() {
  static const base::NoDestructor<std::unordered_map<std::string, std::string>>
      kComputeStyleMap{{
#define DECLARE_PROPERTY_ID(name, c, value) {c, value},
          FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
              {"", ""}}};
  return *kComputeStyleMap;
}

bool CSSProperty::IsInspectorFilteredProperty(CSSPropertyID id) {
  return inspectorFilteredProperties.find(id) !=
         inspectorFilteredProperties.end();
}

}  // namespace tasm
}  // namespace lynx
