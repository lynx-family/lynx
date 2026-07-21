// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_COMPUTED_CSS_STYLE_CSS_TEXT_HELPER_H_
#define CORE_RENDERER_CSS_COMPUTED_CSS_STYLE_CSS_TEXT_HELPER_H_

#include <cstdint>
#include <tuple>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/starlight/types/layout_result.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace tasm {

class ComputedCSSStyleCssTextHelper {
 public:
  ComputedCSSStyleCssTextHelper() = default;
  ~ComputedCSSStyleCssTextHelper() = default;

  base::String GetComputedStyleByPropertyID(
      tasm::CSSPropertyID id, starlight::ComputedCSSStyle* computed_css_style,
      starlight::LayoutResultForRendering ref_layout_result) {
    const auto* funcMap = GetterFuncMap();
    if (id > tasm::CSSPropertyID::kPropertyStart &&
        id < tasm::CSSPropertyID::kPropertyEnd) {
      if (StyleStringValueGetter func = funcMap[id]) {
        return (this->*func)(computed_css_style, ref_layout_result);
      }
    }
    return base::String();
  }

 private:
  using StyleStringValueGetter = base::String (
      ComputedCSSStyleCssTextHelper::*)(starlight::ComputedCSSStyle*,
                                        starlight::LayoutResultForRendering);
  static const StyleStringValueGetter* GetterFuncMap();

#define FOREACH_GET_COMPUTED_VALUE_SUPPORTED_PROPERTY(V) \
  V(Top)                                                 \
  V(Bottom)                                              \
  V(Left)                                                \
  V(Right)                                               \
  V(Width)                                               \
  V(Height)                                              \
  V(Transform)                                           \
  V(TransformOrigin)                                     \
  V(Opacity)                                             \
  V(Color)                                               \
  V(BackgroundColor)                                     \
  V(ZIndex)                                              \
  V(Filter)                                              \
  V(Direction)                                           \
  V(BackgroundPosition)                                  \
  V(Padding)                                             \
  V(PaddingTop)                                          \
  V(PaddingBottom)                                       \
  V(PaddingLeft)                                         \
  V(PaddingRight)                                        \
  V(Margin)                                              \
  V(MarginTop)                                           \
  V(MarginBottom)                                        \
  V(MarginLeft)                                          \
  V(MarginRight)                                         \
  V(BorderWidth)                                         \
  V(BorderTopWidth)                                      \
  V(BorderBottomWidth)                                   \
  V(BorderLeftWidth)                                     \
  V(BorderRightWidth)                                    \
  V(BorderRadius)                                        \
  V(BorderTopLeftRadius)                                 \
  V(BorderTopRightRadius)                                \
  V(BorderBottomRightRadius)                             \
  V(BorderBottomLeftRadius)                              \
  V(BorderColor)                                         \
  V(BorderTopColor)                                      \
  V(BorderRightColor)                                    \
  V(BorderBottomColor)                                   \
  V(BorderLeftColor)

#define GETTER_STYLE_STRING_VALUE(name)                \
  base::String name##CSSText(                          \
      starlight::ComputedCSSStyle* computed_css_style, \
      starlight::LayoutResultForRendering ref_layout_result);
  FOREACH_GET_COMPUTED_VALUE_SUPPORTED_PROPERTY(GETTER_STYLE_STRING_VALUE)
#undef GETTER_STYLE_STRING_VALUE

  base::Vector<base::String> ParseBackgroundPositionArray(
      starlight::ComputedCSSStyle* computed_css_style);

  base::String FloatToPixelString(float value);

  base::String Uint32ToRGBString(uint32_t color_value);

  base::String ConcatStringsWithWhitespace(
      const base::Vector<base::String>& strings);

  base::String BorderRadiusPairCSSText(
      const starlight::NLength& rx, const starlight::NLength& ry,
      const starlight::LayoutResultForRendering& ref_layout_result);

  std::tuple<base::String, base::String> BorderRadiusPairCSSTextComponents(
      const starlight::NLength& rx, const starlight::NLength& ry,
      const starlight::LayoutResultForRendering& ref_layout_result);

  float GetDefaultBorderWidth(starlight::ComputedCSSStyle* computed_css_style);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_COMPUTED_CSS_STYLE_CSS_TEXT_HELPER_H_
