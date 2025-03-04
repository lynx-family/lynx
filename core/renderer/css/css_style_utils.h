// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_STYLE_UTILS_H_
#define CORE_RENDERER_CSS_CSS_STYLE_UTILS_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/layout_unit.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/style/shadow_data.h"
#include "core/style/text_attributes.h"

namespace lynx {
namespace tasm {
class LayoutContext;
class LynxEnvConfig;
class CssMeasureContext;
}  // namespace tasm
namespace starlight {

struct AnimationData;
struct TimingFunctionData;
struct TransformRawData;
struct TransitionData;
struct FilterData;

class NLength;

class CSSStyleUtils {
 public:
  template <typename T>
  static inline void PrepareOptional(std::optional<T>& optional) {
    if (!optional) {
      optional = T();
    }
  }

  template <typename T>
  static inline void PrepareOptional(std::optional<T>& optional,
                                     bool css_align_with_legacy_w3c) {
    if (!optional) {
      optional = T(css_align_with_legacy_w3c);
    }
  }

  static inline void PrepareOptionalForTextAttributes(
      std::optional<starlight::TextAttributes>& optional,
      float default_font_size) {
    if (!optional) {
      optional = starlight::TextAttributes(default_font_size);
    }
  }

  static std::pair<NLength, bool> ToLength(
      const tasm::CSSValue& value, const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs, bool is_font_relevant = false);

  static std::optional<float> ResolveFontSize(
      const tasm::CSSValue& value, const tasm::LynxEnvConfig& config,
      const starlight::LayoutUnit& vw_base,
      const starlight::LayoutUnit& vh_base, double cur_node_font_size,
      double root_node_font_size, const tasm::CSSParserConfigs& configs);

  static float RoundValueToPixelGrid(
      const float value, const float physical_pixels_per_layout_unit);

  // Only air element is using this method now. After air element completes the
  // optimization that flush keyframes by names, this method can be removed.
  static lepus::Value ResolveCSSKeyframes(
      const tasm::CSSKeyframesTokenMap& frames,
      const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs);

  static lepus::Value ResolveCSSKeyframesToken(
      tasm::CSSKeyframesToken* token, const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs);

  static bool ComputeBoolStyle(const tasm::CSSValue& value, const bool reset,
                               bool& dest, const bool default_value,
                               const char* msg,
                               const tasm::CSSParserConfigs& configs);
  static bool ComputeFloatStyle(const tasm::CSSValue& value, const bool reset,
                                float& dest, const float default_value,
                                const char* msg,
                                const tasm::CSSParserConfigs& configs);

  static bool ComputeIntStyle(const tasm::CSSValue& value, const bool reset,
                              int& dest, const int default_value,
                              const char* msg,
                              const tasm::CSSParserConfigs& configs);
  static bool ComputeLengthStyle(const tasm::CSSValue& value, const bool reset,
                                 const tasm::CssMeasureContext& context,
                                 NLength& dest, const NLength& default_value,
                                 const tasm::CSSParserConfigs& configs);
  static bool ComputeGridTrackSizing(
      const tasm::CSSValue& value, const bool reset,
      const tasm::CssMeasureContext& context, std::vector<NLength>& min_dest,
      std::vector<NLength>& max_dest, const std::vector<NLength>& default_value,
      const char* msg, const tasm::CSSParserConfigs& configs);

  template <typename T>
  static bool ComputeEnumStyle(const tasm::CSSValue& value, bool reset, T& dest,
                               const T default_value, const char* msg,
                               const tasm::CSSParserConfigs& configs) {
    auto old_value = dest;
    if (reset) {
      dest = default_value;
    } else {
      CSS_HANDLER_FAIL_IF_NOT(value.IsEnum(), configs.enable_css_strict_mode,
                              msg)
      dest = static_cast<T>(value.GetValue().Number());
    }
    return old_value != dest;
  }

  static bool CalculateLength(const tasm::CSSValue& value, float& result,
                              const tasm::CssMeasureContext& context,
                              const tasm::CSSParserConfigs& configs);

  static void ConvertCSSValueToNumber(const tasm::CSSValue& value,
                                      float& result, PlatformLengthUnit& unit,
                                      const tasm::CssMeasureContext& context,
                                      const tasm::CSSParserConfigs& configs);

  static bool ComputeUIntStyle(const tasm::CSSValue& value, const bool reset,
                               unsigned int& dest,
                               const unsigned int default_value,
                               const char* msg,
                               const tasm::CSSParserConfigs& configs);
  static bool ComputeShadowStyle(const tasm::CSSValue& value, const bool reset,
                                 std::optional<std::vector<ShadowData>>& shadow,
                                 const tasm::CssMeasureContext& context,
                                 const tasm::CSSParserConfigs& configs);

  static bool ComputeTransform(
      const tasm::CSSValue& value, bool reset,
      std::optional<std::vector<TransformRawData>>& raw,
      const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs);

  static bool IsLayoutRelatedTransform(
      const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& style);

  static lepus_value TransformToLepus(
      std::optional<std::vector<TransformRawData>> items);

  static bool ComputeFilter(const tasm::CSSValue& value, bool reset,
                            std::optional<FilterData>& filter,
                            const tasm::CssMeasureContext,
                            const tasm::CSSParserConfigs& configs);

  static lepus_value FilterToLepus(std::optional<FilterData> filter);

  static bool ComputeStringStyle(const tasm::CSSValue& value, const bool reset,
                                 base::String& dest,
                                 const base::String& default_value,
                                 const char* msg,
                                 const tasm::CSSParserConfigs& configs);
  static bool ComputeTimingFunction(const lepus::Value& value, const bool reset,
                                    TimingFunctionData& timing_function,
                                    const tasm::CSSParserConfigs& configs);
  static bool ComputeLongStyle(const tasm::CSSValue& value, const bool reset,
                               long& dest, const long default_value,
                               const char* msg,
                               const tasm::CSSParserConfigs& configs);

  template <typename T, typename F0, typename F1>
  static bool SetAnimationProperty(std::optional<std::vector<T>>& anim,
                                   const tasm::CSSValue& value,
                                   F0 const& reset_func, F1 const& compute_func,
                                   const bool reset,
                                   const tasm::CSSParserConfigs& configs) {
    if (reset) {
      if (anim) {
        for (auto& it : *anim) {
          reset_func(it);
        }
      }
      return true;
    }
    CSS_HANDLER_FAIL_IF_NOT(value.IsEnum() || value.IsNumber() ||
                                value.IsString() || value.IsArray(),
                            configs.enable_css_strict_mode,
                            "Animation or Transition property must "
                            "be enum, number, string or array!")
    CSSStyleUtils::PrepareOptional(anim);
    if (anim->empty()) {
      anim->push_back(T());
    }
    bool changed = false;
    size_t input_size;
    if (value.IsArray()) {
      auto arr = value.GetValue().Array();
      input_size = arr->size();
      for (size_t i = 0; i < arr->size(); i++) {
        if (anim->size() < i + 1) {
          anim->push_back(T());
        }
        changed |= compute_func(arr->get(i), (*anim)[i], reset);
      }
    } else {
      input_size = 1;
      changed = compute_func(value.GetValue(), anim->front(), reset);
    }
    changed = changed || input_size != anim->size();
    // Reset the remaining values
    for (size_t i = input_size; i < anim->size(); ++i) {
      reset_func((*anim)[i]);
    }
    return changed;
  }

  static bool ComputeHeroAnimation(const tasm::CSSValue& value,
                                   const bool reset,
                                   std::optional<AnimationData>& anim,
                                   const char* msg,
                                   const tasm::CSSParserConfigs& configs);
  static bool ComputeAnimation(const lepus::Value& value, AnimationData& anim,
                               const char* msg,
                               const tasm::CSSParserConfigs& configs);
  static lepus_value AnimationDataToLepus(AnimationData& anim);

  static std::shared_ptr<tasm::StyleMap> ProcessCSSAttrsMap(
      const lepus::Value& value, const tasm::CSSParserConfigs& configs);
  static void UpdateCSSKeyframes(tasm::CSSKeyframesTokenMap& keyframes_map,
                                 const std::string& name,
                                 const lepus::Value& keyframes,
                                 const tasm::CSSParserConfigs& configs);
  static float GetBorderWidthFromLengthToFloat(
      const NLength& value, const tasm::CssMeasureContext& context);

  static void AddLengthToArray(const fml::RefPtr<lepus::CArray>& array,
                               const NLength& length);
  static void ComputeBasicShapeEllipse(const fml::RefPtr<lepus::CArray>& raw,
                                       bool reset,
                                       fml::RefPtr<lepus::CArray>& out,
                                       const tasm::CssMeasureContext& context,
                                       const tasm::CSSParserConfigs& configs);
  static void ComputeBasicShapeCircle(const fml::RefPtr<lepus::CArray>& raw,
                                      bool reset,
                                      fml::RefPtr<lepus::CArray>& out,
                                      const tasm::CssMeasureContext& context,
                                      const tasm::CSSParserConfigs& configs);
  static void ComputeBasicShapePath(const fml::RefPtr<lepus::CArray>& raw,
                                    bool reset,
                                    fml::RefPtr<lepus::CArray>& out);
  static void ComputeSuperEllipse(const fml::RefPtr<lepus::CArray>& raw,
                                  bool reset, fml::RefPtr<lepus::CArray>& out,
                                  const tasm::CssMeasureContext& context,
                                  const tasm::CSSParserConfigs& configs);
  static void ComputeBasicShapeInset(const fml::RefPtr<lepus::CArray>& raw,
                                     bool reset,
                                     const fml::RefPtr<lepus::CArray>& dst,
                                     const tasm::CssMeasureContext& context,
                                     const tasm::CSSParserConfigs& configs);

  static bool IsBorderLengthLegal(std::string value);

  static void ComputeRadialGradient(const lepus::Value& gradient_data,
                                    const tasm::CssMeasureContext& context,
                                    const tasm::CSSParserConfigs& configs);

  static lepus::Value GetGradientArrayFromString(
      const char* gradient_def, size_t gradient_def_length,
      const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs);

 private:
  static lepus::Value ResolveCSSKeyframesStyle(
      tasm::StyleMap* attrs, const tasm::CssMeasureContext& context,
      const tasm::CSSParserConfigs& configs);
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_STYLE_UTILS_H_
