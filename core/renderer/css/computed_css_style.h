// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_COMPUTED_CSS_STYLE_H_
#define CORE_RENDERER_CSS_COMPUTED_CSS_STYLE_H_

#include <errno.h>
#include <stdlib.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/starlight/style/box_data.h"
#include "core/renderer/starlight/style/data_ref.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/style/flex_data.h"
#include "core/renderer/starlight/style/grid_data.h"
#include "core/renderer/starlight/style/layout_computed_style.h"
#include "core/renderer/starlight/style/linear_data.h"
#include "core/renderer/starlight/style/relative_data.h"
#include "core/renderer/starlight/style/surround_data.h"
#include "core/renderer/starlight/types/layout_types.h"
#include "core/renderer/tasm/config.h"
#include "core/style/animation_data.h"
#include "core/style/background_data.h"
#include "core/style/default_computed_style.h"
#include "core/style/filter_data.h"
#include "core/style/layout_animation_data.h"
#include "core/style/outline_data.h"
#include "core/style/perspective_data.h"
#include "core/style/shadow_data.h"
#include "core/style/text_attributes.h"
#include "core/style/transform_origin_data.h"
#include "core/style/transform_raw_data.h"
#include "core/style/transition_data.h"

namespace lynx {
namespace starlight {
/** CSSStyle stores the specified values of all CSS properties.
 * Specified values are the values assigned to CSS properties when they are set,
 * including px, %, auto, and various enumerated properties. All CSS properties
 * are grouped. */

class ComputedCSSStyle {
 public:
  BASE_EXPORT ComputedCSSStyle(float layouts_unit_per_px,
                               double physical_pixels_per_layout_unit);
  ComputedCSSStyle(const ComputedCSSStyle& o);
  ~ComputedCSSStyle() = default;

  BASE_EXPORT bool SetValue(tasm::CSSPropertyID id, const tasm::CSSValue& value,
                            bool reset = false);

  double GetFontSize() const { return length_context_.cur_node_font_size_; }

  double GetRootFontSize() const {
    return length_context_.root_node_font_size_;
  }

  void SetScreenWidth(float screen_width) {
    length_context_.screen_width_ = screen_width;
    layout_computed_style_.SetScreenWidth(screen_width);
  }

  bool SetFontScale(float font_scale);

  void SetFontScaleOnlyEffectiveOnSp(bool on_sp) {
    length_context_.font_scale_sp_only_ = on_sp;
  }

  void SetViewportWidth(const LayoutUnit& width) {
    length_context_.viewport_width_ = width;
  }

  void SetViewportHeight(const LayoutUnit& height) {
    length_context_.viewport_height_ = height;
  }

  bool SetFontSize(double cur_node_font_size, double root_node_font_size) {
    if (length_context_.cur_node_font_size_ == cur_node_font_size &&
        length_context_.root_node_font_size_ == root_node_font_size) {
      return false;
    }
    length_context_.cur_node_font_size_ = cur_node_font_size;
    length_context_.root_node_font_size_ = root_node_font_size;
    return true;
  }

  void SetLayoutUnit(float physical_pixels_per_layout_unit,
                     float layouts_unit_per_px) {
    length_context_.physical_pixels_per_layout_unit_ =
        physical_pixels_per_layout_unit;
    length_context_.layouts_unit_per_px_ = layouts_unit_per_px;
    layout_computed_style_.SetPhysicalPixelsPerLayoutUnit(
        physical_pixels_per_layout_unit);
  }

  const tasm::CssMeasureContext& GetMeasureContext() { return length_context_; }

  void Reset();
  void ResetValue(tasm::CSSPropertyID id);
  void SetOverflowDefaultVisible(bool default_overflow_visible);
  OverflowType GetDefaultOverflowType() const {
    return default_overflow_visible_ ? OverflowType::kVisible
                                     : OverflowType::kHidden;
  }
  lepus_value GetValue(tasm::CSSPropertyID id);
  bool InheritValue(tasm::CSSPropertyID id, const ComputedCSSStyle& from);

  OverflowType GetOverflow() const { return overflow_; }

  bool HasAnimation() const { return animation_data_.has_value(); }

  std::vector<AnimationData>& animation_data() {
    CSSStyleUtils::PrepareOptional(animation_data_);
    return *animation_data_;
  }

  bool HasTransform() const { return transform_raw_.has_value(); }

  bool HasTransformOrigin() const { return transform_origin_.has_value(); }

  bool HasTransition() const { return transition_data_.has_value(); }

  bool HasBorderRadius() const {
    return layout_computed_style_.surround_data_.border_data_ &&
           (layout_computed_style_.surround_data_.border_data_
                ->radius_x_top_left.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_x_top_right.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_x_bottom_right.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_x_bottom_left.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_y_top_left.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_y_top_right.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_y_bottom_right.GetRawValue() +
            layout_computed_style_.surround_data_.border_data_
                ->radius_y_bottom_left.GetRawValue()) > 0;
  }

  std::vector<TransitionData>& transition_data() {
    CSSStyleUtils::PrepareOptional(transition_data_);
    return *transition_data_;
  }

  void SetCssAlignLegacyWithW3c(bool value) {
    css_align_with_legacy_w3c_ = value;
  }

  void SetCSSParserConfigs(const tasm::CSSParserConfigs& configs) {
    parser_configs_ = configs;
  }

  int GetZIndex() const { return z_index_; }

  bool HasOpacity() const { return base::FloatsNotEqual(opacity_, 1.0f); }

  const LayoutComputedStyle* GetConstLayoutComputedStyle() const {
    return &layout_computed_style_;
  }

  LayoutComputedStyle* GetLayoutComputedStyle() {
    return &layout_computed_style_;
  }

  void PrepareOptionalForTextAttributes() {
    const float default_font_size =
        DEFAULT_FONT_SIZE_DP * length_context_.layouts_unit_per_px_;
    CSSStyleUtils::PrepareOptionalForTextAttributes(text_attributes_,
                                                    default_font_size);
  }

  XAnimationColorInterpolationType new_animator_interpolation() {
    return new_animator_interpolation_;
  }

  static float SAFE_AREA_INSET_TOP_;
  static float SAFE_AREA_INSET_BOTTOM_;
  static float SAFE_AREA_INSET_LEFT_;
  static float SAFE_AREA_INSET_RIGHT_;

 private:
  using StyleFunc = bool (ComputedCSSStyle::*)(const tasm::CSSValue&,
                                               const bool reset);
  using StyleGetterFunc = lepus_value (ComputedCSSStyle::*)();
  using StyleInheritFunc = bool (ComputedCSSStyle::*)(const ComputedCSSStyle&);
  using StyleInheritFuncMap =
      std::unordered_map<tasm::CSSPropertyID, StyleInheritFunc>;

  static const StyleFunc* FuncMap();
  static const StyleGetterFunc* GetterFuncMap();
  const StyleInheritFuncMap& InheritFuncMap();

  // calc style parameters.
  tasm::CssMeasureContext length_context_;
  bool default_overflow_visible_ = false;
  LayoutComputedStyle layout_computed_style_;

  /***************** css style property ***************************/

  int z_index_{DefaultComputedStyle::DEFAULT_LONG};
  float opacity_{DefaultComputedStyle::DEFAULT_OPACITY};

  OverflowType overflow_{DefaultComputedStyle::DEFAULT_OVERFLOW};
  OverflowType overflow_x_{DefaultComputedStyle::DEFAULT_OVERFLOW};
  OverflowType overflow_y_{DefaultComputedStyle::DEFAULT_OVERFLOW};
  VisibilityType visibility_{DefaultComputedStyle::DEFAULT_VISIBILITY};

  std::optional<AnimationData> enter_transition_data_;
  std::optional<AnimationData> exit_transition_data_;
  std::optional<AnimationData> pause_transition_data_;
  std::optional<AnimationData> resume_transition_data_;
  std::optional<BackgroundData> background_data_;
  std::optional<BackgroundData> mask_data_;
  std::optional<LayoutAnimationData> layout_animation_data_;
  std::optional<OutLineData> outline_;
  std::optional<std::vector<AnimationData>> animation_data_;
  std::optional<std::vector<TransformRawData>> transform_raw_;
  std::optional<std::vector<TransitionData>> transition_data_;
  std::optional<std::vector<ShadowData>> box_shadow_;
  std::optional<TextAttributes> text_attributes_;
  std::optional<TransformOriginData> transform_origin_;
  std::optional<FilterData> filter_;
  std::optional<PerspectiveData> perspective_data_;
  // [type, [url, x, y], type, keyword ]
  std::optional<lepus_value> cursor_;
  // clip-path array [type, args..]
  fml::RefPtr<lepus::CArray> clip_path_{nullptr};
  ImageRenderingType image_rendering_ = ImageRenderingType::kAuto;
  XAppRegionType app_region_ = XAppRegionType::kNone;
  XAnimationColorInterpolationType new_animator_interpolation_ =
      XAnimationColorInterpolationType::kAuto;
  unsigned int handle_color_{0};
  float handle_size_{0.f};

  // this should not in css. But here is only compact old version.
  base::String caret_color_;
  base::String adapt_font_size_;
  base::String content_;

  /************ css style property end ***************************/

  bool css_align_with_legacy_w3c_ = false;

  tasm::CSSParserConfigs parser_configs_;

  void ResetOverflow();

// style setter by CSSValue
#define SET_WITH_CSS_VALUE(name, css_name, default_value) \
  bool Set##name(const tasm::CSSValue& value, const bool reset = false);
  FOREACH_ALL_PROPERTY(SET_WITH_CSS_VALUE)
#undef SET_WITH_CSS_VALUE

// platform style getter
#define FOREACH_PLATFORM_PROPERTY(V)     \
  V(Opacity)                             \
  V(Position)                            \
  V(Overflow)                            \
  V(OverflowX)                           \
  V(OverflowY)                           \
  V(FontSize)                            \
  V(LineHeight)                          \
  V(LetterSpacing)                       \
  V(LineSpacing)                         \
  V(Color)                               \
  V(Background)                          \
  V(BackgroundClip)                      \
  V(BackgroundColor)                     \
  V(BackgroundImage)                     \
  V(BackgroundOrigin)                    \
  V(BackgroundPosition)                  \
  V(BackgroundRepeat)                    \
  V(BackgroundSize)                      \
  V(MaskImage)                           \
  V(MaskSize)                            \
  V(MaskOrigin)                          \
  V(MaskClip)                            \
  V(MaskPosition)                        \
  V(MaskRepeat)                          \
  V(Filter)                              \
  V(BorderLeftColor)                     \
  V(BorderRightColor)                    \
  V(BorderTopColor)                      \
  V(BorderBottomColor)                   \
  V(BorderLeftWidth)                     \
  V(BorderRightWidth)                    \
  V(BorderTopWidth)                      \
  V(BorderBottomWidth)                   \
  V(Transform)                           \
  V(TransformOrigin)                     \
  V(Animation)                           \
  V(AnimationName)                       \
  V(AnimationDuration)                   \
  V(AnimationTimingFunction)             \
  V(AnimationDelay)                      \
  V(AnimationIterationCount)             \
  V(AnimationDirection)                  \
  V(AnimationFillMode)                   \
  V(AnimationPlayState)                  \
  V(LayoutAnimationCreateDuration)       \
  V(LayoutAnimationCreateTimingFunction) \
  V(LayoutAnimationCreateDelay)          \
  V(LayoutAnimationCreateProperty)       \
  V(LayoutAnimationDeleteDuration)       \
  V(LayoutAnimationDeleteTimingFunction) \
  V(LayoutAnimationDeleteDelay)          \
  V(LayoutAnimationDeleteProperty)       \
  V(LayoutAnimationUpdateDuration)       \
  V(LayoutAnimationUpdateTimingFunction) \
  V(LayoutAnimationUpdateDelay)          \
  V(Transition)                          \
  V(TransitionProperty)                  \
  V(TransitionDuration)                  \
  V(TransitionDelay)                     \
  V(TransitionTimingFunction)            \
  V(EnterTransitionName)                 \
  V(ExitTransitionName)                  \
  V(PauseTransitionName)                 \
  V(ResumeTransitionName)                \
  V(Visibility)                          \
  V(BorderLeftStyle)                     \
  V(BorderRightStyle)                    \
  V(BorderTopStyle)                      \
  V(BorderBottomStyle)                   \
  V(OutlineColor)                        \
  V(OutlineStyle)                        \
  V(OutlineWidth)                        \
  V(BoxShadow)                           \
  V(BorderColor)                         \
  V(FontFamily)                          \
  V(CaretColor)                          \
  V(TextShadow)                          \
  V(Direction)                           \
  V(WhiteSpace)                          \
  V(FontWeight)                          \
  V(WordBreak)                           \
  V(FontStyle)                           \
  V(TextAlign)                           \
  V(TextOverflow)                        \
  V(TextDecoration)                      \
  V(TextDecorationColor)                 \
  V(ZIndex)                              \
  V(ImageRendering)                      \
  V(VerticalAlign)                       \
  V(BorderRadius)                        \
  V(BorderTopLeftRadius)                 \
  V(BorderTopRightRadius)                \
  V(BorderBottomRightRadius)             \
  V(BorderBottomLeftRadius)              \
  V(ListMainAxisGap)                     \
  V(ListCrossAxisGap)                    \
  V(Perspective)                         \
  V(Cursor)                              \
  V(TextIndent)                          \
  V(ClipPath)                            \
  V(TextStroke)                          \
  V(TextStrokeWidth)                     \
  V(TextStrokeColor)                     \
  V(XAutoFontSize)                       \
  V(XAutoFontSizePresetSizes)            \
  V(Hyphens)                             \
  V(XAppRegion)                          \
  V(XHandleSize)                         \
  V(XHandleColor)
#define GETTER_STYLE_STRING(name) lepus_value name##ToLepus();
  FOREACH_PLATFORM_PROPERTY(GETTER_STYLE_STRING)
#undef GET_WITH_STRING

// style inherit.
#define FOREACH_PLATFORM_COMPLEX_INHERITABLE_PROPERTY(V) \
  V(LineHeight)                                          \
  V(LetterSpacing)                                       \
  V(LineSpacing)

#define INHERIT_CSS_VALUE(name) \
  bool Inherit##name(const ComputedCSSStyle& from);
  FOREACH_PLATFORM_COMPLEX_INHERITABLE_PROPERTY(INHERIT_CSS_VALUE)
#undef INHERIT_CSS_VALUE

 private:
  float GetBorderFinalWidth(float width, BorderStyleType style) const {
    return (style != BorderStyleType::kNone && style != BorderStyleType::kHide)
               ? width
               : 0.f;
  }

};  // ComputedCSSStyle

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_COMPUTED_CSS_STYLE_H_
