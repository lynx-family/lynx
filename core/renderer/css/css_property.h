// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_PROPERTY_H_
#define CORE_RENDERER_CSS_CSS_PROPERTY_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/linked_hash_map.h"
#include "base/include/no_destructor.h"
#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property_id.h"
#include "core/renderer/css/css_value.h"

namespace lynx {
namespace tasm {

using PseudoState = uint32_t;
static constexpr PseudoState kPseudoStateNone = 0;
static constexpr PseudoState kPseudoStateHover = 1;
static constexpr PseudoState kPseudoStateHoverTransition = 1 << 1;
static constexpr PseudoState kPseudoStateActive = 1 << 3;
static constexpr PseudoState kPseudoStateActiveTransition = 1 << 4;
static constexpr PseudoState kPseudoStateFocus = 1 << 6;
static constexpr PseudoState kPseudoStateFocusTransition = 1 << 7;
static constexpr PseudoState kPseudoStatePlaceHolder = 1 << 8;
static constexpr PseudoState kPseudoStateBefore = 1 << 9;
static constexpr PseudoState kPseudoStateAfter = 1 << 10;
static constexpr PseudoState kPseudoStateSelection = 1 << 11;

#define FOREACH_ALL_ANIMATIONAPI_PROPERTY(V) \
  V(AnimationDuration, "duration")           \
  V(AnimationDelay, "delay")                 \
  V(AnimationIterationCount, "iterations")   \
  V(AnimationFillMode, "fill")               \
  V(AnimationTimingFunction, "easing")       \
  V(AnimationDirection, "direction")         \
  V(AnimationPlayState, "play-state")

#define ALL_ANIMATABLE_PROPERTY_ID                                         \
  tasm::kPropertyIDTop, tasm::kPropertyIDLeft, tasm::kPropertyIDRight,     \
      tasm::kPropertyIDBottom, tasm::kPropertyIDWidth,                     \
      tasm::kPropertyIDHeight, tasm::kPropertyIDBackgroundColor,           \
      tasm::kPropertyIDColor, tasm::kPropertyIDOpacity,                    \
      tasm::kPropertyIDBorderLeftColor, tasm::kPropertyIDBorderRightColor, \
      tasm::kPropertyIDBorderTopColor, tasm::kPropertyIDBorderBottomColor, \
      tasm::kPropertyIDBorderLeftWidth, tasm::kPropertyIDBorderRightWidth, \
      tasm::kPropertyIDBorderTopWidth, tasm::kPropertyIDBorderBottomWidth, \
      tasm::kPropertyIDPaddingLeft, tasm::kPropertyIDPaddingRight,         \
      tasm::kPropertyIDPaddingTop, tasm::kPropertyIDPaddingBottom,         \
      tasm::kPropertyIDMarginLeft, tasm::kPropertyIDMarginRight,           \
      tasm::kPropertyIDMarginTop, tasm::kPropertyIDMarginBottom,           \
      tasm::kPropertyIDMaxWidth, tasm::kPropertyIDMinWidth,                \
      tasm::kPropertyIDMaxHeight, tasm::kPropertyIDMinHeight,              \
      tasm::kPropertyIDFlexGrow, tasm::kPropertyIDFlexBasis,               \
      tasm::kPropertyIDFilter, tasm::kPropertyIDTransform

#define FOREACH_NEW_ANIMATOR_PROPERTY(V)              \
  V(kPropertyIDLeft, kLeft)                           \
  V(kPropertyIDTop, kTop)                             \
  V(kPropertyIDRight, kRight)                         \
  V(kPropertyIDBottom, kBottom)                       \
  V(kPropertyIDWidth, kWidth)                         \
  V(kPropertyIDHeight, kHeight)                       \
  V(kPropertyIDOpacity, kOpacity)                     \
  V(kPropertyIDBackgroundColor, kBackgroundColor)     \
  V(kPropertyIDColor, kColor)                         \
  V(kPropertyIDMaxWidth, kMaxWidth)                   \
  V(kPropertyIDMinWidth, kMinWidth)                   \
  V(kPropertyIDMaxHeight, kMaxHeight)                 \
  V(kPropertyIDMinHeight, kMinHeight)                 \
  V(kPropertyIDMarginLeft, kMarginLeft)               \
  V(kPropertyIDMarginRight, kMarginRight)             \
  V(kPropertyIDMarginTop, kMarginTop)                 \
  V(kPropertyIDMarginBottom, kMarginBottom)           \
  V(kPropertyIDPaddingLeft, kPaddingLeft)             \
  V(kPropertyIDPaddingRight, kPaddingRight)           \
  V(kPropertyIDPaddingTop, kPaddingTop)               \
  V(kPropertyIDPaddingBottom, kPaddingBottom)         \
  V(kPropertyIDBorderLeftWidth, kBorderLeftWidth)     \
  V(kPropertyIDBorderRightWidth, kBorderRightWidth)   \
  V(kPropertyIDBorderTopWidth, kBorderTopWidth)       \
  V(kPropertyIDBorderBottomWidth, kBorderBottomWidth) \
  V(kPropertyIDBorderLeftColor, kBorderLeftColor)     \
  V(kPropertyIDBorderRightColor, kBorderRightColor)   \
  V(kPropertyIDBorderTopColor, kBorderTopColor)       \
  V(kPropertyIDBorderBottomColor, kBorderBottomColor) \
  V(kPropertyIDFlexGrow, kFlexGrow)                   \
  V(kPropertyIDFlexBasis, kFlexBasis)                 \
  V(kPropertyIDFilter, kFilter)                       \
  V(kPropertyIDTransform, kTransform)

// Define macro for direction aware property, {css_id, is_logic_style, css_id in
// ltr mode, css_id in rtl mode}
// TODO(zhouzhitao): unify logic with radon element, use this macro to replace
// RTLDirectionMapping defined in dynamic_css_style_manager
#define FOREACH_DIRECTION_MAPPING_PROPERTY(V)                                  \
  V(MarginInlineStart, true, kPropertyIDMarginLeft, kPropertyIDMarginRight)    \
  V(MarginInlineEnd, true, kPropertyIDMarginRight, kPropertyIDMarginLeft)      \
  V(PaddingInlineStart, true, kPropertyIDPaddingLeft, kPropertyIDPaddingRight) \
  V(PaddingInlineEnd, true, kPropertyIDPaddingRight, kPropertyIDPaddingLeft)   \
  V(BorderInlineStartWidth, true, kPropertyIDBorderLeftWidth,                  \
    kPropertyIDBorderRightWidth)                                               \
  V(BorderInlineEndWidth, true, kPropertyIDBorderRightWidth,                   \
    kPropertyIDBorderLeftWidth)                                                \
  V(BorderInlineStartStyle, true, kPropertyIDBorderLeftStyle,                  \
    kPropertyIDBorderRightStyle)                                               \
  V(BorderInlineEndStyle, true, kPropertyIDBorderRightStyle,                   \
    kPropertyIDBorderLeftStyle)                                                \
  V(BorderInlineStartColor, true, kPropertyIDBorderLeftColor,                  \
    kPropertyIDBorderRightColor)                                               \
  V(BorderInlineEndColor, true, kPropertyIDBorderRightColor,                   \
    kPropertyIDBorderLeftColor)                                                \
  V(BorderStartStartRadius, true, kPropertyIDBorderTopLeftRadius,              \
    kPropertyIDBorderTopRightRadius)                                           \
  V(BorderStartEndRadius, true, kPropertyIDBorderTopRightRadius,               \
    kPropertyIDBorderTopLeftRadius)                                            \
  V(BorderEndStartRadius, true, kPropertyIDBorderBottomLeftRadius,             \
    kPropertyIDBorderBottomRightRadius)                                        \
  V(BorderEndEndRadius, true, kPropertyIDBorderBottomRightRadius,              \
    kPropertyIDBorderBottomLeftRadius)                                         \
  V(RelativeAlignInlineStart, true, kPropertyIDRelativeAlignLeft,              \
    kPropertyIDRelativeAlignRight)                                             \
  V(RelativeAlignInlineEnd, true, kPropertyIDRelativeAlignRight,               \
    kPropertyIDRelativeAlignLeft)                                              \
  V(RelativeInlineStartOf, true, kPropertyIDRelativeLeftOf,                    \
    kPropertyIDRelativeRightOf)                                                \
  V(RelativeInlineEndOf, true, kPropertyIDRelativeRightOf,                     \
    kPropertyIDRelativeLeftOf)                                                 \
  V(InsetInlineStart, true, kPropertyIDLeft, kPropertyIDRight)                 \
  V(InsetInlineEnd, true, kPropertyIDRight, kPropertyIDLeft)                   \
  V(MarginLeft, false, kPropertyIDMarginLeft, kPropertyIDMarginRight)          \
  V(MarginRight, false, kPropertyIDMarginRight, kPropertyIDMarginLeft)         \
  V(Left, false, kPropertyIDLeft, kPropertyIDRight)                            \
  V(Right, false, kPropertyIDRight, kPropertyIDLeft)                           \
  V(PaddingLeft, false, kPropertyIDPaddingLeft, kPropertyIDPaddingRight)       \
  V(PaddingRight, false, kPropertyIDPaddingRight, kPropertyIDPaddingLeft)      \
  V(BorderLeftWidth, false, kPropertyIDBorderLeftWidth,                        \
    kPropertyIDBorderRightWidth)                                               \
  V(BorderRightWidth, false, kPropertyIDBorderRightWidth,                      \
    kPropertyIDBorderLeftWidth)                                                \
  V(BorderLeftStyle, false, kPropertyIDBorderLeftStyle,                        \
    kPropertyIDBorderRightStyle)                                               \
  V(BorderRightStyle, false, kPropertyIDBorderRightStyle,                      \
    kPropertyIDBorderLeftStyle)                                                \
  V(BorderLeftColor, false, kPropertyIDBorderLeftColor,                        \
    kPropertyIDBorderRightColor)                                               \
  V(BorderRightColor, false, kPropertyIDBorderRightColor,                      \
    kPropertyIDBorderLeftColor)                                                \
  V(BorderTopLeftRadius, false, kPropertyIDBorderTopLeftRadius,                \
    kPropertyIDBorderTopRightRadius)                                           \
  V(BorderTopRightRadius, false, kPropertyIDBorderTopRightRadius,              \
    kPropertyIDBorderTopLeftRadius)                                            \
  V(BorderBottomLeftRadius, false, kPropertyIDBorderBottomLeftRadius,          \
    kPropertyIDBorderBottomRightRadius)                                        \
  V(BorderBottomRightRadius, false, kPropertyIDBorderBottomRightRadius,        \
    kPropertyIDBorderBottomLeftRadius)                                         \
  V(RelativeAlignLeft, false, kPropertyIDRelativeAlignLeft,                    \
    kPropertyIDRelativeAlignRight)                                             \
  V(RelativeAlignRight, false, kPropertyIDRelativeAlignRight,                  \
    kPropertyIDRelativeAlignLeft)                                              \
  V(RelativeLeftOf, false, kPropertyIDRelativeLeftOf,                          \
    kPropertyIDRelativeRightOf)                                                \
  V(RelativeRightOf, false, kPropertyIDRelativeRightOf,                        \
    kPropertyIDRelativeLeftOf)

using StyleMap = base::LinkedHashMap<CSSPropertyID, tasm::CSSValue>;
using CSSVariableMap = base::LinkedHashMap<base::String, base::String>;
using ParsedStyles = std::pair<StyleMap, CSSVariableMap>;
using ParsedStylesMap =
    std::unordered_map<std::string, std::shared_ptr<ParsedStyles>>;

using AirCompStylesMap =
    std::unordered_map<std::string, std::shared_ptr<StyleMap>>;
using AirParsedStylesMap = std::unordered_map<std::string, AirCompStylesMap>;

using RawStyleMap = base::LinkedHashMap<CSSPropertyID, tasm::CSSValue>;
using RawLepusStyleMap = base::LinkedHashMap<CSSPropertyID, lepus::Value>;

constexpr int kCSSPropertyCount = kPropertyEnd;

/* Sometimes, for example, when setting inline styles on nodes one by one
 through the render function, we cannot get the exact number of styles, so we
 provide a fuzzy initial capacity for the StyleMap that stores these styles.
 For most scenarios, we can ensure that the StyleMap stores data in the same
 continuous memory without wasting too much memory.
 This is just a magic number balanced between memory usage and performance.
 */
constexpr size_t kCSSStyleMapFuzzyAllocationSize = 6;

class BASE_EXPORT_FOR_DEVTOOL CSSProperty {
 public:
  // base::String, const char* and std::string could be implicitly converted
  // to base::static_string::GenericCacheKey very cheaply.
  static CSSPropertyID GetPropertyID(
      const base::static_string::GenericCacheKey& key);

  static const base::static_string::GenericCache& GetPropertyName(
      CSSPropertyID id);

  static const char* GetPropertyNameCStr(CSSPropertyID id) {
    return GetPropertyName(id).c_str();
  }

  // Get total count of properties after parsing [id] if [id] is a shorthand
  // one. If [id] is not a shorthand property this function returns 0.
  static size_t GetShorthandExpand(CSSPropertyID id);

  // Input map may contain shorthand properties. This function calculates total
  // count of properties after parsing the whole map.
  template <class Map>
  static typename std::enable_if<
      std::is_same_v<typename Map::key_type, CSSPropertyID>, size_t>::type
  GetTotalParsedStyleCountFromMap(const Map& map) {
    // Shorthand raw styles are decomposed to multiple ones and precalculate
    // reserving count for target map from source map.
    size_t reserve_count = map.size();
    for (const auto& kv_pair : map) {
      if (auto expand = GetShorthandExpand(kv_pair.first); expand > 0) {
        reserve_count += expand - 1;
      }
    }
    return reserve_count;
  }

  template <class Array, class Trait = typename Array::TraitID>
  static size_t GetTotalParsedStyleCountFromArray(const Array* array,
                                                  size_t size) {
    size_t reserve_count = size;
    for (size_t i = 0; i < size; i++) {
      if (auto expand = GetShorthandExpand(Trait::GetPropertyID(array[i]));
          expand > 0) {
        reserve_count += expand - 1;
      }
    }
    return reserve_count;
  }

  static size_t GetTotalParsedStyleCountFromArray(const CSSPropertyID* array,
                                                  size_t size) {
    size_t reserve_count = size;
    for (size_t i = 0; i < size; i++) {
      if (auto expand = GetShorthandExpand(array[i]); expand > 0) {
        reserve_count += expand - 1;
      }
    }
    return reserve_count;
  }

  static inline bool IsPropertyValid(
      const base::static_string::GenericCacheKey& name) {
    return IsPropertyValid(GetPropertyID(name));
  }

  static inline bool IsPropertyValid(CSSPropertyID id) {
    return id > kPropertyStart && id < kPropertyEnd;
  }

  // When using Element animation api, the timing options's keys are not
  // standard css expressions. In order to get the corresponding PropertyID, add
  // the following function.
  static CSSPropertyID GetTimingOptionsPropertyID(
      const base::static_string::GenericCacheKey& key);

  static inline bool IsTransitionProps(CSSPropertyID id) {
    return id >= CSSPropertyID::kPropertyIDTransition &&
           id <= CSSPropertyID::kPropertyIDTransitionTimingFunction;
  }

  static inline bool IsKeyframeProps(CSSPropertyID id) {
    return id >= CSSPropertyID::kPropertyIDAnimation &&
           id <= CSSPropertyID::kPropertyIDAnimationPlayState;
  }

 private:
  CSSProperty() = delete;

 public:
  static const std::unordered_map<std::string, std::string>&
  GetComputeStyleMap();
  static bool IsShorthand(CSSPropertyID id);
};

}  // namespace tasm
}  // namespace lynx

namespace std {
template <>
struct hash<lynx::tasm::CSSPropertyID> {
  std::size_t operator()(const lynx::tasm::CSSPropertyID& k) const {
    return static_cast<std::size_t>(k);
  }
};
}  // namespace std

#endif  // CORE_RENDERER_CSS_CSS_PROPERTY_H_
