// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_style_utils.h"

#include <cmath>

#include "base/include/compiler_specific.h"
#include "base/include/debug/lynx_assert.h"
#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/include/vector.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/style/animation_data.h"
#include "core/style/color.h"
#include "core/style/default_computed_style.h"
#include "core/style/filter_data.h"
#include "core/style/layout_animation_data.h"
#include "core/style/transform_origin_data.h"
#include "core/style/transform_raw_data.h"
#include "core/style/transition_data.h"

namespace lynx {
namespace starlight {
#define TRUE_RETURN(result) \
  do {                      \
    type = result;          \
    return true;            \
  } while (0);

#define FALSE_RETURN(css_name)                               \
  do {                                                       \
    LOGE("invalid value for " << css_name << ": " << value); \
    return false;                                            \
  } while (0);

namespace {
namespace {
constexpr const char* kViewWidth = "view_width";
constexpr const char* kViewHeight = "view_height";
constexpr const char* k100VH = "100vh";
constexpr const char* k100VW = "100vw";
}  // namespace
constexpr float kRpxRatio = 750.0f;
struct CalcValue {
  float unit_value = 0.0;
  float per_value = 0.0;
  float number_value = 0.0;

  bool is_number = true;
};

std::pair<NLength, bool> TryMakeIntrinsicNLength(
    const std::string& value_str, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (value_str == "max-content") {
    return std::pair<NLength, bool>(NLength::MakeMaxContentNLength(), true);
  } else if (value_str == "fit-content") {
    return std::pair<NLength, bool>(NLength::MakeFitContentNLength(), true);
  } else if (base::BeginsWith(value_str, "fit-content(") &&
             base::EndsWith(value_str, ")")) {
    // get xxx from fit-content(xxx);
    std::string sub_value = value_str.substr(12, value_str.length() - 13);
    tasm::CSSValue css_value;
    lynx::tasm::LengthHandler::Process(lepus::Value(std::move(sub_value)),
                                       css_value, configs);
    auto result = CSSStyleUtils::ToLength(css_value, context, configs);
    return std::pair<NLength, bool>(
        NLength::MakeFitContentNLength(result.first.NumericLength()), true);
  }

  return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
}

std::pair<int, bool> GetEnvValue(const std::string& env_name) {
  if (env_name == "safe-area-inset-top") {
    return std::make_pair(
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_TOP_, true);
  } else if (env_name == "safe-area-inset-bottom") {
    return std::make_pair(
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_, true);
  } else if (env_name == "safe-area-inset-left") {
    return std::make_pair(
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_LEFT_, true);
  } else if (env_name == "safe-area-inset-right") {
    return std::make_pair(
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_, true);
  }

  return std::make_pair(0, false);
}

const size_t kExprCalcInlineStackSize = 16;
using CalcValueStack = base::InlineStack<CalcValue, kExprCalcInlineStackSize>;

bool CalculationTopTwoData(CalcValueStack& data_stack, char operation) {
  if (data_stack.size() < 2) {
    return false;
  }

  // stack
  CalcValue data2 = data_stack.top();
  data_stack.pop();
  CalcValue data1 = data_stack.top();
  data_stack.pop();

  if (operation == '+') {
    if (data1.is_number != data2.is_number) {
      return false;
    }
    data1.unit_value += data2.unit_value;
    data1.per_value += data2.per_value;
    data1.number_value += data2.number_value;
    data_stack.push(std::move(data1));
  } else if (operation == '-') {
    if (data1.is_number != data2.is_number) {
      return false;
    }
    if (data1.is_number) {
      data1.number_value -= data2.number_value;
    } else {
      data1.unit_value -= data2.unit_value;
      data1.per_value -= data2.per_value;
    }
    data_stack.push(std::move(data1));
  } else if (operation == '*') {
    // One data should be a number
    if (!data1.is_number && !data2.is_number) {
      return false;
    }
    if (data1.is_number && data2.is_number) {
      data1.number_value *= data2.number_value;
      data_stack.push(std::move(data1));
    } else {
      float number = data1.is_number ? data1.number_value : data2.number_value;
      CalcValue& length = data1.is_number ? data2 : data1;
      length.unit_value *= number;
      length.per_value *= number;
      data_stack.push(std::move(length));
    }
  } else if (operation == '/') {
    if (!data2.is_number || data2.number_value == 0) {
      return false;
    }

    if (data1.is_number) {
      data1.number_value /= data2.number_value;
    } else {
      data1.unit_value /= data2.number_value;
      data1.per_value /= data2.number_value;
    }
    data_stack.push(std::move(data1));
  } else {
    return false;
  }

  return true;
}

std::pair<NLength, bool> TryMakeCalcNLength(
    const std::string& value_str, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs, bool is_font_relevant) {
  if (value_str.find("auto") != std::string::npos ||
      !base::BeginsWith(value_str, "calc")) {
    return std::pair<NLength, bool>(NLength::MakeAutoNLength(), true);
  }

  size_t value_len = value_str.length();
  // Operations include + - * / ( )
  base::InlineStack<char, kExprCalcInlineStackSize> op_stack;
  // Data stack
  CalcValueStack data_stack;
  // value cache
  std::string sub_value;

  // skip "calc"
  for (size_t i = 4; i < value_len; ++i) {
    char tchar = value_str[i];
    bool is_operation = (tchar == '*' || tchar == '/' || tchar == '(' ||
                         tchar == ')' || tchar == '+' || tchar == '-');
    // parse sub length
    if ((is_operation || tchar == ' ') && !sub_value.empty()) {
      tasm::CSSValue css_value;
      if (sub_value == kViewWidth) {
        sub_value = k100VW;

      } else if (sub_value == kViewHeight) {
        sub_value = k100VH;
      }
      if (!lynx::tasm::LengthHandler::Process(lepus::Value(sub_value),
                                              css_value, configs)) {
        return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
      }
      CalcValue value;
      if (css_value.GetPattern() == tasm::CSSValuePattern::NUMBER) {
        value.is_number = true;
        value.number_value = static_cast<float>(css_value.GetValue().Number());
      } else {
        std::pair<NLength, bool> result = CSSStyleUtils::ToLength(
            css_value, context, configs, is_font_relevant);
        if (!result.second) {
          return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
        }
        value.is_number = false;
        if (result.first.GetType() == NLengthType::kNLengthUnit) {
          value.unit_value = result.first.GetRawValue();
        } else if (result.first.GetType() == NLengthType::kNLengthPercentage) {
          value.per_value = result.first.GetRawValue();
        } else {
          return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
        }
      }
      data_stack.push(std::move(value));
      sub_value.clear();
    }

    if (tchar == ' ') {
      continue;
    }

    // The four basic operations.
    // There are two space around add and sub sign.
    if (is_operation &&
        (!(tchar == '+' || tchar == '-') ||
         (i > 0 && i < value_len - 1 && value_str[i - 1] == ' ' &&
          value_str[i + 1] == ' '))) {
      if (tchar == '+' || tchar == '-') {
        while (!op_stack.empty() && op_stack.top() != '(') {
          if (!CalculationTopTwoData(data_stack, op_stack.top())) {
            return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
          }
          op_stack.pop();
        }
        op_stack.push(tchar);
      } else if (tchar == '*' || tchar == '/') {
        while (!op_stack.empty() &&
               (op_stack.top() == '*' || op_stack.top() == '/')) {
          if (!CalculationTopTwoData(data_stack, op_stack.top())) {
            return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
          }
          op_stack.pop();
        }
        op_stack.push(tchar);
      } else if (tchar == '(') {
        op_stack.push(tchar);
      } else if (tchar == ')') {
        while (!op_stack.empty() && op_stack.top() != '(') {
          if (!CalculationTopTwoData(data_stack, op_stack.top())) {
            return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
          }
          op_stack.pop();
        }
        // remove kLeftBracket
        if (op_stack.empty()) {
          return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
        }
        op_stack.pop();
      }
      continue;
    }

    sub_value += tchar;

    if (sub_value == "env") {
      size_t env_end_index = i + 1;
      while (env_end_index < value_len && value_str[env_end_index] != ')') {
        ++env_end_index;
      }
      if (env_end_index == value_len) {
        return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
      }

      std::string env_func =
          sub_value + value_str.substr(i + 1, env_end_index - i);
      tasm::CSSValue css_value;
      if (!lynx::tasm::LengthHandler::Process(lepus::Value(std::move(env_func)),
                                              css_value, configs)) {
        return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
      }
      std::pair<NLength, bool> result =
          CSSStyleUtils::ToLength(css_value, context, configs);
      if (result.second) {
        CalcValue env_value;
        env_value.is_number = false;
        env_value.unit_value = result.first.GetRawValue();
        data_stack.push(CalcValue(env_value));
      } else {
        return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
      }
      // skip env function
      sub_value.clear();
      i = env_end_index;
      continue;
    }
  }
  if (op_stack.size() != 0 || data_stack.size() != 1) {
    return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
  }

  std::optional<NLength> calc_length;

  // FIXME(zhixuan): calc(0%) should behave differrent from calc(0)
  if (!base::FloatsEqual(data_stack.top().per_value, 0.0f)) {
    return std::pair<NLength, bool>(
        NLength::MakeCalcNLength(data_stack.top().unit_value,
                                 data_stack.top().per_value),
        true);
  } else {
    return std::pair<NLength, bool>(
        NLength::MakeCalcNLength(data_stack.top().unit_value), true);
  }
}
}  // namespace

namespace {

std::pair<NLength, bool> ToLengthHelper(const tasm::CSSValue& value,
                                        const float& factor) {
  float float_value = static_cast<float>(value.GetValue().Number()) * factor;
  return std::pair<NLength, bool>(NLength::MakeUnitNLength(float_value), true);
}

void ComputeShadowStyleHelper(float& prop_result, const base::String& key,
                              const fml::RefPtr<lepus::Dictionary>& dict,
                              const tasm::CssMeasureContext& context,
                              const tasm::CSSParserConfigs& configs) {
  auto prop_arr = dict->GetValue(key).Array();
  auto prop = tasm::CSSValue(
      prop_arr->get(0),
      static_cast<tasm::CSSValuePattern>(prop_arr->get(1).Number()));
  CSSStyleUtils::CalculateLength(prop, prop_result, context, configs);
}

void SetX1Y1(TimingFunctionData& timing_function,
             const fml::RefPtr<lepus::CArray>& arr) {
  timing_function.x1 = arr->get(TimingFunctionData::INDEX_X1).Number();
  timing_function.y1 = arr->get(TimingFunctionData::INDEX_Y1).Number();
}

void UpdateAnimationProp(lepus::Value& p, long& dest,
                         const tasm::CSSPropertyID anim_id,
                         const fml::RefPtr<lepus::Dictionary>& map) {
  p = map->GetValue(std::to_string(anim_id));
  if (p.IsNumber()) {
    dest = static_cast<long>(p.Number());
  }
}
}  // namespace

// TODO(zhixuan): return base length here.
std::pair<NLength, bool> CSSStyleUtils::ToLength(
    const tasm::CSSValue& value, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs, bool is_font_relevant) {
  auto pattern = value.GetPattern();
  const float non_sp_font_scale =
      (is_font_relevant && !context.font_scale_sp_only_) ? context.font_scale_
                                                         : 1.f;

  if (pattern == tasm::CSSValuePattern::NUMBER) {
    float float_value =
        static_cast<float>(value.GetValue().Number()) * non_sp_font_scale;
    return std::pair<NLength, bool>(NLength::MakeUnitNLength(float_value),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::PX) {
    float float_value = static_cast<float>(value.GetValue().Number()) *
                        context.layouts_unit_per_px_ * non_sp_font_scale;
    return std::pair<NLength, bool>(NLength::MakeUnitNLength(float_value),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::RPX) {
    float float_value = static_cast<float>(value.GetValue().Number()) *
                        context.screen_width_ / kRpxRatio * non_sp_font_scale;
    return std::pair<NLength, bool>(NLength::MakeUnitNLength(float_value),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::PPX) {
    float float_value = static_cast<float>(value.GetValue().Number()) /
                        context.physical_pixels_per_layout_unit_ *
                        non_sp_font_scale;
    return std::pair<NLength, bool>(NLength::MakeUnitNLength(float_value),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::REM) {
    return ToLengthHelper(value, context.root_node_font_size_);
  } else if (pattern == tasm::CSSValuePattern::EM) {
    return ToLengthHelper(value, context.cur_node_font_size_);
  } else if (pattern == tasm::CSSValuePattern::PERCENT) {
    float float_value = static_cast<float>(value.GetValue().Number());
    return std::pair<NLength, bool>(NLength::MakePercentageNLength(float_value),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::VH) {
    return context.viewport_height_.IsDefinite()
               ? ToLengthHelper(value,
                                context.viewport_height_.ToFloat() / 100.f)
               : std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
  } else if (pattern == tasm::CSSValuePattern::VW) {
    return context.viewport_width_.IsDefinite()
               ? ToLengthHelper(value,
                                context.viewport_width_.ToFloat() / 100.f)
               : std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
  } else if (pattern == tasm::CSSValuePattern::CALC) {
    const auto& value_old = value.GetValue().StdString();
    return TryMakeCalcNLength(value_old, context, configs, is_font_relevant);
  } else if (pattern == tasm::CSSValuePattern::INTRINSIC) {
    const auto& value_old = value.GetValue().StdString();
    return TryMakeIntrinsicNLength(value_old, context, configs);
  } else if (pattern == tasm::CSSValuePattern::ENV) {
    const auto& value_old = value.GetValue().StdString();
    size_t len = value_old.length();
    auto env_name = value_old.substr(4, len - 5);
    auto found = env_name.find_first_not_of(' ');
    if (found != std::string::npos) env_name.erase(0, found);
    found = env_name.find_last_not_of(' ');
    if (found != std::string::npos) env_name.erase(found + 1);
    std::pair<int, bool> result = GetEnvValue(env_name);
    if (!result.second) {
      return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
    }
    return std::pair<NLength, bool>(NLength::MakeUnitNLength(result.first),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::ENUM) {
    return std::pair<NLength, bool>(NLength::MakeAutoNLength(), true);
  } else if (pattern == tasm::CSSValuePattern::SP) {
    float float_value = static_cast<float>(value.GetValue().Number()) *
                        context.layouts_unit_per_px_ * context.font_scale_;
    return std::pair<NLength, bool>(NLength::MakeUnitNLength(float_value),
                                    true);
  } else if (pattern == tasm::CSSValuePattern::FR) {
    float float_value = static_cast<float>(value.GetValue().Number());
    return std::pair<NLength, bool>(NLength::MakeFrNLength(float_value), true);
  } else {
    tasm::UnitHandler::CSSWarning(false, configs.enable_css_strict_mode,
                                  (std::string("no such type length:") +
                                   std::to_string(static_cast<int>(pattern)))
                                      .c_str());
    return std::pair<NLength, bool>(NLength::MakeAutoNLength(), false);
  }
}

std::optional<float> CSSStyleUtils::ResolveFontSize(
    const tasm::CSSValue& value, const tasm::LynxEnvConfig& env_config,
    const starlight::LayoutUnit& vw_base, const starlight::LayoutUnit& vh_base,
    double cur_node_font_size, double root_node_font_size,
    const tasm::CSSParserConfigs& configs) {
  tasm::CssMeasureContext css_context(env_config, root_node_font_size,
                                      cur_node_font_size);
  css_context.viewport_width_ = vw_base;
  css_context.viewport_height_ = vh_base;
  css_context.font_scale_sp_only_ = env_config.FontScaleSpOnly();

  std::optional<float> result;
  const auto resolved_result = ToLength(value, css_context, configs, true);

  if (resolved_result.second) {
    const auto resolved_unit = starlight::NLengthToLayoutUnit(
        resolved_result.first,
        starlight::LayoutUnit(css_context.cur_node_font_size_));
    if (resolved_unit.IsDefinite()) {
      result = resolved_unit.ToFloat();
    }
  }
  return result;
}

float CSSStyleUtils::RoundValueToPixelGrid(
    const float value, const float physical_pixels_per_layout_unit) {
  return std::roundf(value * physical_pixels_per_layout_unit) /
         physical_pixels_per_layout_unit;
}

lepus::Value CSSStyleUtils::ResolveCSSKeyframesStyle(
    tasm::StyleMap* attrs, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  fml::RefPtr<lepus::Dictionary> dict = lepus::Dictionary::Create();
  for (const auto& [key, value] : *attrs) {
    if (key == tasm::kPropertyIDBackgroundColor ||
        key == tasm::kPropertyIDColor) {
      unsigned int color = 0;
      ComputeUIntStyle(value, false, color, DefaultColor::DEFAULT_COLOR,
                       "background-color must be a number!", configs);
      dict->SetValue(tasm::CSSProperty::GetPropertyName(key), color);
    } else if (key == tasm::kPropertyIDOpacity) {
      float opacity = 1.0f;
      ComputeFloatStyle(value, false, opacity,
                        DefaultComputedStyle::DEFAULT_FLOAT,
                        "opacity must be a float!", configs);
      dict->SetValue(tasm::CSSProperty::GetPropertyName(key), opacity);
    } else if (key == tasm::kPropertyIDTransform) {
      // transform
      std::optional<std::vector<TransformRawData>> raw =
          std::vector<TransformRawData>();
      ComputeTransform(value, false, raw, context, configs);
      dict->SetValue(tasm::CSSProperty::GetPropertyName(key),
                     TransformToLepus(raw));
    } else if (key == tasm::kPropertyIDLeft || key == tasm::kPropertyIDTop ||
               key == tasm::kPropertyIDWidth ||
               key == tasm::kPropertyIDHeight) {
      dict->SetValue(tasm::CSSProperty::GetPropertyName(key), value.GetValue());
    } else {
      tasm::UnitHandler::CSSWarning(false, configs.enable_css_strict_mode,
                                    "keyframe don't support id:%d", key);
    }
  }
  return lepus::Value(std::move(dict));
}

lepus::Value CSSStyleUtils::ResolveCSSKeyframesToken(
    tasm::CSSKeyframesToken* token, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  auto dict = lepus::Dictionary::Create();
  for (const auto& [key, value] : token->GetKeyframesContent()) {
    dict->SetValue(std::to_string(key),
                   ResolveCSSKeyframesStyle(value.get(), context, configs));
  }
  return lepus::Value(std::move(dict));
}

lepus::Value CSSStyleUtils::ResolveCSSKeyframes(
    const tasm::CSSKeyframesTokenMap& frames,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  auto dict = lepus::Dictionary::Create();
  for (const auto& [key, value] : frames) {
    dict->SetValue(key,
                   ResolveCSSKeyframesToken(value.get(), context, configs));
  }
  return lepus::Value(std::move(dict));
}

bool CSSStyleUtils::ComputeBoolStyle(const tasm::CSSValue& value,
                                     const bool reset, bool& dest,
                                     const bool default_value, const char* msg,
                                     const tasm::CSSParserConfigs& configs) {
  auto old_value = dest;
  if (reset) {
    dest = default_value;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.IsBoolean(), configs.enable_css_strict_mode,
                            msg)
    dest = value.GetValue().Bool();
  }
  return old_value != dest;
}

namespace {

template <typename T>
inline bool ComputeNumberStyle(const tasm::CSSValue& value, const bool reset,
                               T& dest, const T default_value, const char* msg,
                               const tasm::CSSParserConfigs& configs) {
  auto old_value = dest;
  if (reset) {
    dest = default_value;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.IsNumber(), configs.enable_css_strict_mode,
                            msg)
    dest = static_cast<T>(value.GetValue().Number());
  }
  if (!base::FloatsEqual(old_value, dest)) {
    return true;
  }
  dest = old_value;
  return false;
}

template <>
inline bool ComputeNumberStyle(const tasm::CSSValue& value, const bool reset,
                               uint32_t& dest, const uint32_t default_value,
                               const char* msg,
                               const tasm::CSSParserConfigs& configs) {
  auto old_value = dest;
  if (reset) {
    dest = default_value;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.GetValue().IsNumber(),
                            configs.enable_css_strict_mode, msg)
    dest = static_cast<uint32_t>(value.GetValue().Number());
  }
  return old_value != dest;
}

}  // namespace

bool CSSStyleUtils::ComputeFloatStyle(const tasm::CSSValue& value,
                                      const bool reset, float& dest,
                                      const float default_value,
                                      const char* msg,
                                      const tasm::CSSParserConfigs& configs) {
  return ComputeNumberStyle<float>(value, reset, dest, default_value, msg,
                                   configs);
}

bool CSSStyleUtils::ComputeIntStyle(const tasm::CSSValue& value,
                                    const bool reset, int& dest,
                                    const int default_value, const char* msg,
                                    const tasm::CSSParserConfigs& configs) {
  return ComputeNumberStyle<int>(value, reset, dest, default_value, msg,
                                 configs);
}

bool CSSStyleUtils::ComputeUIntStyle(const tasm::CSSValue& value,
                                     const bool reset, unsigned int& dest,
                                     const unsigned int default_value,
                                     const char* msg,
                                     const tasm::CSSParserConfigs& configs) {
  return ComputeNumberStyle<unsigned int>(value, reset, dest, default_value,
                                          msg, configs);
}

bool CSSStyleUtils::ComputeGridTrackSizing(
    const tasm::CSSValue& value, const bool reset,
    const tasm::CssMeasureContext& context, std::vector<NLength>& min_dest,
    std::vector<NLength>& max_dest, const std::vector<NLength>& default_value,
    const char* msg, const tasm::CSSParserConfigs& configs) {
  auto old_min_value = min_dest;
  auto old_max_value = max_dest;
  if (reset) {
    min_dest = default_value;
    max_dest = default_value;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.IsArray(), configs.enable_css_strict_mode,
                            msg)
    auto length_array = value.GetValue().Array();
    std::vector<NLength> length_arr_min_result;
    std::vector<NLength> length_arr_max_result;

    for (size_t idx = 0; idx < length_array->size(); idx += 2) {
      tasm::CSSValue css_value(length_array->get(idx),
                               static_cast<tasm::CSSValuePattern>(
                                   length_array->get(idx + 1).Number()));
      // Handle minmax function.
      // e.g., minmax(10px, max-content) -->
      // (CSSFunctionType::MINMAX, CSSValuePattern::ENUM),
      // (10, CSSValuePattern::PX),
      // ("max-content", CSSValuePattern::INTRINSIC)
      if (css_value.GetPattern() == tasm::CSSValuePattern::ENUM &&
          static_cast<tasm::CSSFunctionType>(css_value.GetValue().Number()) ==
              tasm::CSSFunctionType::MINMAX) {
        idx += 2;
        if (idx + 3 >= length_array->size()) {
          return false;
        }
        tasm::CSSValue css_value(length_array->get(idx),
                                 static_cast<tasm::CSSValuePattern>(
                                     length_array->get(idx + 1).Number()));
        std::pair<NLength, bool> result =
            CSSStyleUtils::ToLength(css_value, context, configs);
        length_arr_min_result.emplace_back(result.first);
        idx += 2;
        css_value = tasm::CSSValue(length_array->get(idx),
                                   static_cast<tasm::CSSValuePattern>(
                                       length_array->get(idx + 1).Number()));
        result = CSSStyleUtils::ToLength(css_value, context, configs);
        length_arr_max_result.emplace_back(result.first);
      } else {
        std::pair<NLength, bool> result =
            CSSStyleUtils::ToLength(css_value, context, configs);
        length_arr_min_result.emplace_back(result.first);
        length_arr_max_result.emplace_back(result.first);
      }
    }
    min_dest = std::move(length_arr_min_result);
    max_dest = std::move(length_arr_max_result);
  }

  return old_min_value != min_dest || old_max_value != max_dest;
}

bool CSSStyleUtils::ComputeLengthStyle(const tasm::CSSValue& value,
                                       const bool reset,
                                       const tasm::CssMeasureContext& context,
                                       NLength& dest,
                                       const NLength& default_value,
                                       const tasm::CSSParserConfigs& configs) {
  NLength old_value = dest;
  if (reset) {
    dest = default_value;
  } else {
    auto parse_result = CSSStyleUtils::ToLength(value, context, configs);
    if (!parse_result.second) {
      return false;
    }
    dest = std::move(parse_result.first);
  }
  return old_value != dest;
}

bool CSSStyleUtils::CalculateLength(const tasm::CSSValue& value, float& result,
                                    const tasm::CssMeasureContext& context,
                                    const tasm::CSSParserConfigs& configs) {
  auto parse_result = CSSStyleUtils::ToLength(value, context, configs);
  if (!parse_result.second) {
    return false;
  }

  const auto& length = parse_result.first;
  result = CSSStyleUtils::RoundValueToPixelGrid(
      length.GetRawValue(), context.physical_pixels_per_layout_unit_);
  return true;
}

void CSSStyleUtils::ConvertCSSValueToNumber(
    const tasm::CSSValue& value, float& result, PlatformLengthUnit& unit,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (value.IsPercent()) {
    result = value.AsNumber() / 100.0f;
    unit = PlatformLengthUnit::PERCENTAGE;
  } else {
    CalculateLength(value, result, context, configs);
  }
}

void GetLengthData(NLength& length, const lepus_value& value,
                   const lepus_value& unit,
                   const tasm::CssMeasureContext& context,
                   const tasm::CSSParserConfigs& configs) {
  auto pattern = static_cast<tasm::CSSValuePattern>(unit.Number());
  auto parse_result =
      CSSStyleUtils::ToLength(tasm::CSSValue(value, pattern), context, configs);
  if (parse_result.second) {
    length = parse_result.first;
    if (length.IsUnit()) {
      length = NLength::MakeUnitNLength(CSSStyleUtils::RoundValueToPixelGrid(
          length.GetRawValue(), context.physical_pixels_per_layout_unit_));
    }
  }
}

bool CSSStyleUtils::ComputeFilter(const tasm::CSSValue& value, bool reset,
                                  std::optional<FilterData>& filter,
                                  const tasm::CssMeasureContext context,
                                  const tasm::CSSParserConfigs& configs) {
  auto last_filter = filter;
  if (reset) {
    // reset the optional directly, if optional is std::nullopt, filterToLepus
    // will return an empty array.
    filter.reset();
  } else {
    PrepareOptional(filter);
    CSS_HANDLER_FAIL_IF_NOT(value.IsArray(), configs.enable_css_strict_mode,
                            "filter must be an array! [type, length, unit]")
    FilterData item;
    auto attributes = value.GetValue().Array();
    // Check attr size
    CSS_HANDLER_FAIL_IF_NOT(attributes->size() != 0,
                            configs.enable_css_strict_mode,
                            "filter array must have attributes")
    item.type = static_cast<FilterType>(
        attributes->get(FilterData::kIndexType).Number());

    // Check param size.
    CSS_HANDLER_FAIL_IF_NOT(
        item.type == FilterType::kNone || attributes->size() == 3,
        configs.enable_css_strict_mode, "filter function should has a param")
    // Compose unit and number value into NLength.
    GetLengthData(item.amount, attributes->get(FilterData::kIndexAmount),
                  attributes->get(FilterData::kIndexUnit), context, configs);
    *filter = item;
  }
  return last_filter != filter;
}

namespace {
void GetTransformMatrix(TransformType matrix_type,
                        fml::RefPtr<lepus::CArray>& arr,
                        TransformRawData& transform_raw_data,
                        const tasm::CssMeasureContext& context,
                        const tasm::CSSParserConfigs& configs) {
  if (matrix_type == TransformType::kMatrix) {
    for (int i = 0; i < 6; ++i) {
      if (i == 4 || i == 5) {
        // handle translate pixel unit.
        transform_raw_data
            .matrix[TransformRawData::INDEX_2D_TO_3D_MATRIX_ID[i]] =
            CSSStyleUtils::ToLength(
                tasm::CSSValue(arr->get(i + 1), tasm::CSSValuePattern::PX),
                context, configs)
                .first.GetRawValue();
      } else {
        transform_raw_data
            .matrix[TransformRawData::INDEX_2D_TO_3D_MATRIX_ID[i]] =
            arr->get(i + 1).Number();
      }
    }
  } else if (matrix_type == TransformType::kMatrix3d) {
    for (int i = 0; i < 16; ++i) {
      if (i == 12 || i == 13 || i == 14) {
        // handle translate pixel unit.
        transform_raw_data.matrix[TransformRawData::INDEX_3D_MATRIX_ID[i]] =
            CSSStyleUtils::ToLength(
                tasm::CSSValue(arr->get(i + 1), tasm::CSSValuePattern::PX),
                context, configs)
                .first.GetRawValue();
      } else {
        transform_raw_data.matrix[TransformRawData::INDEX_3D_MATRIX_ID[i]] =
            arr->get(i + 1).Number();
      }
    }
  }
  transform_raw_data.matrix_empty = false;
}
}  // namespace

bool CSSStyleUtils::ComputeTransform(
    const tasm::CSSValue& value, bool reset,
    std::optional<std::vector<TransformRawData>>& raw,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  auto old_raw = raw;
  if (reset) {
    raw.reset();
  } else {
    PrepareOptional(raw);
    raw->clear();
    CSS_HANDLER_FAIL_IF_NOT(value.IsArray(), configs.enable_css_strict_mode,
                            "transform must be a array!")
    auto items = value.GetValue().Array();
    CSS_HANDLER_FAIL_IF_NOT(items->size() > 0, configs.enable_css_strict_mode,
                            "transform's array size must > 0")
    raw->reserve(items->size());
    for (size_t i = 0; i < items->size(); i++) {
      CSS_HANDLER_FAIL_IF_NOT(items->get(i).IsArray(),
                              configs.enable_css_strict_mode,
                              "transform's items must be an array")
      auto arr = items->get(i).Array();
      CSS_HANDLER_FAIL_IF_NOT(items->size() > 0, configs.enable_css_strict_mode,
                              "transform's array size must > 0")
      TransformRawData item;
      item.type = static_cast<TransformType>(
          arr->get(TransformRawData::INDEX_FUNC).Number());
      switch (item.type) {
        case TransformType::kTranslate:
          GetLengthData(item.p0, arr->get(TransformRawData::INDEX_TRANSLATE_0),
                        arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT),
                        context, configs);
          item.unit_type0 = static_cast<tasm::CSSValuePattern>(
              arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT).Number());
          if (arr->size() > TransformRawData::INDEX_TRANSLATE_1) {
            GetLengthData(item.p1,
                          arr->get(TransformRawData::INDEX_TRANSLATE_1),
                          arr->get(TransformRawData::INDEX_TRANSLATE_1_UNIT),
                          context, configs);
            item.unit_type1 = static_cast<tasm::CSSValuePattern>(
                arr->get(TransformRawData::INDEX_TRANSLATE_1_UNIT).Number());
          }
          break;
        case TransformType::kTranslateX:
        case TransformType::kTranslateY:
        case TransformType::kTranslateZ:
          GetLengthData(item.p0, arr->get(TransformRawData::INDEX_TRANSLATE_0),
                        arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT),
                        context, configs);
          item.unit_type0 = static_cast<tasm::CSSValuePattern>(
              arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT).Number());
          break;
        case TransformType::kTranslate3d:
          GetLengthData(item.p0, arr->get(TransformRawData::INDEX_TRANSLATE_0),
                        arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT),
                        context, configs);
          item.unit_type0 = static_cast<tasm::CSSValuePattern>(
              arr->get(TransformRawData::INDEX_TRANSLATE_0_UNIT).Number());
          GetLengthData(item.p1, arr->get(TransformRawData::INDEX_TRANSLATE_1),
                        arr->get(TransformRawData::INDEX_TRANSLATE_1_UNIT),
                        context, configs);
          item.unit_type1 = static_cast<tasm::CSSValuePattern>(
              arr->get(TransformRawData::INDEX_TRANSLATE_1_UNIT).Number());
          GetLengthData(item.p2, arr->get(TransformRawData::INDEX_TRANSLATE_2),
                        arr->get(TransformRawData::INDEX_TRANSLATE_2_UNIT),
                        context, configs);
          item.unit_type2 = static_cast<tasm::CSSValuePattern>(
              arr->get(TransformRawData::INDEX_TRANSLATE_2_UNIT).Number());
          break;
        case TransformType::kRotate:
        case TransformType::kRotateX:
        case TransformType::kRotateY:
        case TransformType::kRotateZ:
          item.p0 = NLength::MakeUnitNLength(
              arr->get(TransformRawData::INDEX_ROTATE_ANGLE).Number());
          break;
        case TransformType::kScale:
          item.p0 = NLength::MakeUnitNLength(
              arr->get(TransformRawData::INDEX_SCALE_0).Number());
          if (arr->size() <= TransformRawData::INDEX_SCALE_1) {
            item.p1 = item.p0;
          } else {
            item.p1 = NLength::MakeUnitNLength(
                arr->get(TransformRawData::INDEX_SCALE_1).Number());
          }
          break;
        case TransformType::kScaleX:
        case TransformType::kScaleY:
          item.p0 = NLength::MakeUnitNLength(
              arr->get(TransformRawData::INDEX_SCALE_0).Number());
          break;
        case TransformType::kSkew:
          item.p0 = NLength::MakeUnitNLength(
              arr->get(TransformRawData::INDEX_SKEW_0).Number());
          if (arr->size() <= TransformRawData::INDEX_SKEW_1) {
            item.p1 = NLength::MakeUnitNLength(.0f);
          } else {
            item.p1 = NLength::MakeUnitNLength(
                arr->get(TransformRawData::INDEX_SKEW_1).Number());
          }
          break;
        case TransformType::kSkewX:
        case TransformType::kSkewY:
          item.p0 = NLength::MakeUnitNLength(
              arr->get(TransformRawData::INDEX_SKEW_0).Number());
          break;
        case TransformType::kMatrix:
        case TransformType::kMatrix3d:
          GetTransformMatrix(item.type, arr, item, context, configs);
          break;
        default:
          LynxWarning(false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
                      "can't reach here, no such instance:%d", (int)item.type);
          break;
      }
      raw->push_back(item);
    }
  }
  return old_raw != raw;
}

/// Generate a lepus array to platform according to the computed `FilterData`.
/// - Parameter filter: Current filter state.
/// - Returns: A lepus array, [int, double, int], indicates [FilterType, Amount,
/// Unit]. Empty if `filter` is nullopt, which typically occurs when the value
/// is reset.
lepus_value CSSStyleUtils::FilterToLepus(std::optional<FilterData> filter) {
  auto result = lepus::CArray::Create();
  if (filter) {
    result->emplace_back(static_cast<int>(filter->type));
    // Transfer NLength into platform unit value.
    AddLengthToArray(result, filter->amount);
  }
  return lepus_value(std::move(result));
}

lepus_value CSSStyleUtils::TransformToLepus(
    std::optional<std::vector<TransformRawData>> transform_raw) {
  auto items = lepus::CArray::Create();
  for (auto& tr : *transform_raw) {
    auto item = lepus::CArray::Create();
    item->emplace_back(static_cast<int>(tr.type));
    if (tr.type == TransformType::kMatrix ||
        tr.type == TransformType::kMatrix3d) {
      for (int i = 0; i < 16; ++i) {
        item->emplace_back(tr.matrix[i]);
      }
    } else {
      AddLengthToArray(item, tr.p0);
      AddLengthToArray(item, tr.p1);
      AddLengthToArray(item, tr.p2);
    }
    items->emplace_back(std::move(item));
  }
  return lepus_value(std::move(items));
}

bool CSSStyleUtils::IsLayoutRelatedTransform(
    const std::pair<tasm::CSSPropertyID, tasm::CSSValue>& style) {
  if (style.first == tasm::kPropertyIDTransform) {
    auto array = style.second.GetValue().Array();
    for (size_t idx = 0; idx < array->size(); ++idx) {
      auto transform = array->get(idx).Array();
      auto transform_func =
          static_cast<starlight::TransformType>(transform->get(0).Int32());
      if (transform_func == starlight::TransformType::kTranslate ||
          transform_func == starlight::TransformType::kTranslateX ||
          transform_func == starlight::TransformType::kTranslateY ||
          transform_func == starlight::TransformType::kTranslateZ ||
          transform_func == starlight::TransformType::kTranslate3d) {
        for (size_t param_idx = 2; param_idx < transform->size();
             param_idx += 2) {
          auto pattern = static_cast<tasm::CSSValuePattern>(
              transform->get(param_idx).Int32());
          if (pattern == tasm::CSSValuePattern::RPX ||
              pattern == tasm::CSSValuePattern::EM ||
              pattern == tasm::CSSValuePattern::REM ||
              pattern == tasm::CSSValuePattern::VW ||
              pattern == tasm::CSSValuePattern::VH) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool CSSStyleUtils::ComputeStringStyle(const tasm::CSSValue& value,
                                       const bool reset, base::String& dest,
                                       const base::String& default_value,
                                       const char* msg,
                                       const tasm::CSSParserConfigs& configs) {
  auto old_value = dest;
  if (reset) {
    dest = default_value;
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.IsString(), configs.enable_css_strict_mode,
                            msg)
    dest = value.GetValue().String();
  }
  return !old_value.IsEqual(dest);
}

bool CSSStyleUtils::ComputeTimingFunction(
    const lepus::Value& value, const bool reset,
    TimingFunctionData& timing_function,
    const tasm::CSSParserConfigs& configs) {
  auto old_value = timing_function;
  if (reset) {
    timing_function.Reset();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(value.IsNumber() || value.IsArray(),
                            configs.enable_css_strict_mode,
                            "timing-function must be a enum or a array!")
    if (value.IsNumber()) {
      timing_function.timing_func =
          static_cast<TimingFunctionType>(value.Number());
    } else {
      auto arr = value.Array();
      timing_function.timing_func = static_cast<TimingFunctionType>(
          arr->get(TimingFunctionData::INDEX_TYPE).Number());
      if (timing_function.timing_func == TimingFunctionType::kSquareBezier) {
        SetX1Y1(timing_function, arr);
      } else if (timing_function.timing_func ==
                 TimingFunctionType::kCubicBezier) {
        SetX1Y1(timing_function, arr);
        timing_function.x2 = arr->get(TimingFunctionData::INDEX_X2).Number();
        timing_function.y2 = arr->get(TimingFunctionData::INDEX_Y2).Number();
      } else if (timing_function.timing_func == TimingFunctionType::kSteps) {
        timing_function.x1 = arr->get(TimingFunctionData::INDEX_X1).Number();
        timing_function.steps_type = static_cast<StepsType>(
            arr->get(TimingFunctionData::INDEX_STEPS_TYPE).Number());
      } else {
        std::string error_msg =
            std::string("no such bezier implementation") +
            std::to_string(static_cast<int>(timing_function.timing_func));
        LynxWarning(false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
                    error_msg.c_str());
      }
    }
  }
  return !(old_value == timing_function);
}

bool CSSStyleUtils::ComputeLongStyle(const tasm::CSSValue& value,
                                     const bool reset, long& dest,
                                     const long default_value, const char* msg,
                                     const tasm::CSSParserConfigs& configs) {
  return ComputeNumberStyle<long>(value, reset, dest, default_value, msg,
                                  configs);
}

bool CSSStyleUtils::ComputeHeroAnimation(
    const tasm::CSSValue& value, const bool reset,
    std::optional<AnimationData>& anim, const char* msg,
    const tasm::CSSParserConfigs& configs) {
  auto old_value = anim ? *anim : DefaultComputedStyle::DEFAULT_ANIMATION();
  if (reset) {
    anim.reset();
  } else {
    if (value.IsEmpty()) {
      return false;
    }
    CSS_HANDLER_FAIL_IF_NOT(value.IsArray() || value.IsMap(),
                            configs.enable_css_strict_mode, msg)
    PrepareOptional(anim);
    if (value.IsArray()) {
      auto array = value.GetValue().Array();
      if (array->size() == 0) {
        return false;
      }
      ComputeAnimation(array->get(0), *anim, msg, configs);
    } else {
      ComputeAnimation(value.GetValue(), *anim, msg, configs);
    }
  }

  return old_value != anim;
}

bool CSSStyleUtils::ComputeAnimation(const lepus::Value& value,
                                     AnimationData& anim, const char* msg,
                                     const tasm::CSSParserConfigs& configs) {
  CSS_HANDLER_FAIL_IF_NOT(value.IsObject(), configs.enable_css_strict_mode, msg)
  auto map = value.Table();
  auto p = map->GetValue(std::to_string(tasm::kPropertyIDAnimationName));
  if (p.IsString()) {
    anim.name = p.String();
  }

  UpdateAnimationProp(p, anim.duration, tasm::kPropertyIDAnimationDuration,
                      map);
  p = map->GetValue(std::to_string(tasm::kPropertyIDAnimationTimingFunction));
  if (p.IsArray()) {
    ComputeTimingFunction(p.Array()->get(0), false, anim.timing_func, configs);
  }

  p = map->GetValue(std::to_string(tasm::kPropertyIDAnimationFillMode));
  if (p.IsNumber()) {
    anim.fill_mode = static_cast<starlight::AnimationFillModeType>(p.Number());
  }

  UpdateAnimationProp(p, anim.delay, tasm::kPropertyIDAnimationDelay, map);

  p = map->GetValue(std::to_string(tasm::kPropertyIDAnimationDirection));
  if (p.IsNumber()) {
    anim.direction = static_cast<starlight::AnimationDirectionType>(p.Number());
  }
  p = map->GetValue(std::to_string(tasm::kPropertyIDAnimationIterationCount));
  if (p.IsNumber()) {
    anim.iteration_count = static_cast<int>(p.Number());
  }
  p = map->GetValue(std::to_string(tasm::kPropertyIDAnimationPlayState));
  if (p.IsNumber()) {
    anim.play_state =
        static_cast<starlight::AnimationPlayStateType>(p.Number());
  }

  return true;
}

lepus_value CSSStyleUtils::AnimationDataToLepus(AnimationData& anim) {
  auto array = lepus::CArray::Create();
  array->emplace_back(anim.name);
  array->emplace_back(static_cast<double>(anim.duration));
  array->emplace_back(static_cast<int>(anim.timing_func.timing_func));
  array->emplace_back(static_cast<int>(anim.timing_func.steps_type));
  array->emplace_back(static_cast<float>(anim.timing_func.x1));
  array->emplace_back(static_cast<float>(anim.timing_func.y1));
  array->emplace_back(static_cast<float>(anim.timing_func.x2));
  array->emplace_back(static_cast<float>(anim.timing_func.y2));
  array->emplace_back(static_cast<double>(anim.delay));
  array->emplace_back(static_cast<int>(anim.iteration_count));
  array->emplace_back(static_cast<int>(anim.direction));
  array->emplace_back(static_cast<int>(anim.fill_mode));
  array->emplace_back(static_cast<int>(anim.play_state));
  return lepus::Value(std::move(array));
}

bool CSSStyleUtils::ComputeShadowStyle(
    const tasm::CSSValue& value, const bool reset,
    std::optional<std::vector<ShadowData>>& shadow,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (reset) {
    shadow.reset();
    return true;
  }
  auto old_value =
      shadow ? *shadow : DefaultComputedStyle::DEFAULT_BOX_SHADOW();
  CSS_HANDLER_FAIL_IF_NOT(value.IsArray(), configs.enable_css_strict_mode,
                          "shadow must be an array!")
  auto group = value.GetValue().Array();
  std::vector<ShadowData> dest;
  BASE_STATIC_STRING_DECL(kEnable, "enable");
  for (size_t i = 0; i < group->size(); i++) {
    auto dict = group->get(i).Table();
    bool enable = true;
    if (dict->Contains(kEnable)) {
      enable = dict->GetValue(kEnable).Bool();
    }
    if (enable) {
      CSS_HANDLER_FAIL_IF_NOT(dict->size() > 2, configs.enable_css_strict_mode,
                              "shadow must have h_offset and v_offset !")
      if (dest.size() < (i + 1)) {
        dest.emplace_back(ShadowData());
      }
      auto& shadow_ele = dest.at(i);

      BASE_STATIC_STRING_DECL(kHOffset, "h_offset");
      ComputeShadowStyleHelper(shadow_ele.h_offset, kHOffset, dict, context,
                               configs);

      BASE_STATIC_STRING_DECL(kVOffset, "v_offset");
      ComputeShadowStyleHelper(shadow_ele.v_offset, kVOffset, dict, context,
                               configs);

      BASE_STATIC_STRING_DECL(kBlur, "blur");
      if (dict->Contains(kBlur)) {
        ComputeShadowStyleHelper(shadow_ele.blur, kBlur, dict, context,
                                 configs);
      }
      BASE_STATIC_STRING_DECL(kSpread, "spread");
      if (dict->Contains(kSpread)) {
        ComputeShadowStyleHelper(shadow_ele.spread, kSpread, dict, context,
                                 configs);
      }
      BASE_STATIC_STRING_DECL(kOption, "option");
      if (dict->Contains(kOption)) {
        auto option = dict->GetValue(kOption).Number();
        shadow_ele.option = static_cast<ShadowOption>(option);
      }
      BASE_STATIC_STRING_DECL(kColor, "color");
      if (dict->Contains(kColor)) {
        auto color = static_cast<uint32_t>(dict->GetValue(kColor).Number());
        shadow_ele.color = color;
      }
    }
  }
  if (dest.size() > 0) {
    shadow = dest;
  } else {
    shadow.reset();
  }
  return old_value !=
         (shadow ? *shadow : DefaultComputedStyle::DEFAULT_BOX_SHADOW());
}

std::shared_ptr<tasm::StyleMap> CSSStyleUtils::ProcessCSSAttrsMap(
    const lepus::Value& value, const tasm::CSSParserConfigs& configs) {
  auto map = std::make_shared<tasm::StyleMap>();
  if (!value.IsObject()) {
    return map;
  }
  const auto& table = value.Table();
  map->reserve(table->size());
  for (const auto& [key, value] : *table) {
    tasm::CSSPropertyID id = tasm::CSSProperty::GetPropertyID(key);
    if (!tasm::CSSProperty::IsPropertyValid(id)) {
      continue;
    }
    tasm::UnitHandler::Process(id, value, *(map.get()), configs);
  }
  return map;
}

void CSSStyleUtils::UpdateCSSKeyframes(
    tasm::CSSKeyframesTokenMap& keyframes_map, const std::string& name,
    const lepus::Value& keyframes, const tasm::CSSParserConfigs& configs) {
  if (!keyframes.IsTable()) {
    if (!keyframes.IsArray() || keyframes.Array()->size() < 2) {
      return;
    }
    keyframes_map[name] = std::make_unique<tasm::CSSKeyframesToken>(configs);
    const auto& ary = keyframes.Array();
    float interval = 1.0f / (ary->size() - 1);
    for (size_t i = 0; i < ary->size(); ++i) {
      keyframes_map[name]->GetKeyframesContent().insert(std::make_pair(
          i * interval, ProcessCSSAttrsMap(ary->get(i), configs)));
    }
    return;
  }
  if (keyframes.Table()->size() < 2) {
    return;
  }
  keyframes_map[name] = std::make_unique<tasm::CSSKeyframesToken>(configs);
  auto table = keyframes.Table();
  for (auto& iter : *table) {
    const std::string& per = iter.first.str();
    if (per.empty()) {
      continue;
    }
    char* endptr = nullptr;
    float interval = strtof(per.c_str(), &endptr) / 100.0;
    keyframes_map[name]->GetKeyframesContent().insert(std::make_pair(
        interval,
        starlight::CSSStyleUtils::ProcessCSSAttrsMap(iter.second, configs)));
  }
}

float CSSStyleUtils::GetBorderWidthFromLengthToFloat(
    const NLength& value, const tasm::CssMeasureContext& context) {
  float raw_value = value.NumericLength().GetFixedPart();
  return CSSStyleUtils::RoundValueToPixelGrid(
      raw_value, context.physical_pixels_per_layout_unit_);
}

/**
 * Add a NLength value to a lepus::CArray, append the value and unit to the
 * target array. Convert the NLength to [value, unit] for the platform.
 * We don't know the parent length value, so need to add the sub lengths to
 * array for calc length.
 * @param array target array, to which append the value and unit.
 * @param length the length to be added
 */
void CSSStyleUtils::AddLengthToArray(const fml::RefPtr<lepus::CArray>& array,
                                     const NLength& length) {
  const auto push_length = [](const fml::RefPtr<lepus::CArray>& array,
                              const NLength& length) {
    if (length.NumericLength().ContainsPercentage()) {
      array->emplace_back(length.NumericLength().GetPercentagePart() / 100.f);
      array->emplace_back(static_cast<int>(PlatformLengthUnit::PERCENTAGE));
    }
    if (length.NumericLength().ContainsFixedValue() ||
        !length.NumericLength().ContainsPercentage()) {
      array->emplace_back(length.NumericLength().GetFixedPart());
      array->emplace_back(static_cast<int>(PlatformLengthUnit::NUMBER));
    }
  };
  if (length.IsCalc() && length.NumericLength().ContainsFixedValue() &&
      length.NumericLength().ContainsPercentage()) {
    auto calc = lepus::CArray::Create();
    push_length(calc, length);
    array->emplace_back(std::move(calc));
    array->emplace_back(static_cast<int>(PlatformLengthUnit::CALC));
  } else {
    push_length(array, length);
  }
}

/**
 *  Compute the basic shape ellipse function to lepus::array.
 *  [type ellipse, radiusX, platformUnit, radiusY,  platformUnit, centerX,
 * platformUnit, centerY, platformUnit]
 * @param raw array, [type, radiusX, unit, radiusY, unit, centerX, unit,
 * centerY, unit]
 * @param out output array, empty array created outside the function.
 * @param context length context
 */
void CSSStyleUtils::ComputeBasicShapeEllipse(
    const fml::RefPtr<lepus::CArray>& raw, bool reset,
    fml::RefPtr<lepus::CArray>& out, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (reset) {
    // Keep the array empty.
    return;
  }
  constexpr int INDEX_ELLIPSE_TYPE = 0;
  constexpr int INDEX_ELLIPSE_RADIUS_X = 1;
  constexpr int INDEX_ELLIPSE_RADIUS_X_UNIT = 2;
  constexpr int INDEX_ELLIPSE_RADIUS_Y = 3;
  constexpr int INDEX_ELLIPSE_RADIUS_Y_UNIT = 4;
  constexpr int INDEX_ELLIPSE_CENTER_X = 5;
  constexpr int INDEX_ELLIPSE_CENTER_X_UNIT = 6;
  constexpr int INDEX_ELLIPSE_CENTER_Y = 7;
  constexpr int INDEX_ELLIPSE_CENTER_Y_UNIT = 8;

  out->push_back(raw->get(INDEX_ELLIPSE_TYPE));
  NLength radius_x = NLength::MakeAutoNLength(),
          radius_y = NLength::MakeAutoNLength(),
          center_x = NLength::MakeAutoNLength(),
          center_y = NLength::MakeAutoNLength();
  // Compute the CSSValue to NLength according to unit and length context.
  GetLengthData(radius_x, raw->get(INDEX_ELLIPSE_RADIUS_X),
                raw->get(INDEX_ELLIPSE_RADIUS_X_UNIT), context, configs);
  GetLengthData(radius_y, raw->get(INDEX_ELLIPSE_RADIUS_Y),
                raw->get(INDEX_ELLIPSE_RADIUS_Y_UNIT), context, configs);
  GetLengthData(center_x, raw->get(INDEX_ELLIPSE_CENTER_X),
                raw->get(INDEX_ELLIPSE_CENTER_X_UNIT), context, configs);
  GetLengthData(center_y, raw->get(INDEX_ELLIPSE_CENTER_Y),
                raw->get(INDEX_ELLIPSE_CENTER_Y_UNIT), context, configs);

  // Change the unit to platform unit and append to target array.
  AddLengthToArray(out, radius_x);
  AddLengthToArray(out, radius_y);
  AddLengthToArray(out, center_x);
  AddLengthToArray(out, center_y);
}

/**
 * Compute the radius and position in basic shape circle array.
 * @param raw array contains radius, centerX and centerY
 * @param out output array, 1-D lepus array stores the [type, radius,
 * platformUnit, centerX, platformUnit, centerY, platformUnit]
 */
void CSSStyleUtils::ComputeBasicShapeCircle(
    const fml::RefPtr<lepus::CArray>& raw, bool reset,
    fml::RefPtr<lepus::CArray>& out, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (reset) {
    // Keep the array empty.
    return;
  }

  constexpr int INDEX_CIRCLE_TYPE = 0;
  constexpr int INDEX_CIRCLE_RADIUS = 1;
  constexpr int INDEX_CIRCLE_RADIUS_UNIT = 2;
  constexpr int INDEX_CIRCLE_CENTER_X = 3;
  constexpr int INDEX_CIRCLE_CENTER_X_UNIT = 4;
  constexpr int INDEX_CIRCLE_CENTER_Y = 5;
  constexpr int INDEX_CIRCLE_CENTER_Y_UNIT = 6;

  out->push_back(raw->get(INDEX_CIRCLE_TYPE));
  NLength radius = NLength::MakeAutoNLength(),
          center_x = NLength::MakeAutoNLength(),
          center_y = NLength::MakeAutoNLength();
  GetLengthData(radius, raw->get(INDEX_CIRCLE_RADIUS),
                raw->get(INDEX_CIRCLE_RADIUS_UNIT), context, configs);
  GetLengthData(center_x, raw->get(INDEX_CIRCLE_CENTER_X),
                raw->get(INDEX_CIRCLE_CENTER_X_UNIT), context, configs);
  GetLengthData(center_y, raw->get(INDEX_CIRCLE_CENTER_Y),
                raw->get(INDEX_CIRCLE_CENTER_Y_UNIT), context, configs);

  // Convert unit to platformUnit and append to output array.
  AddLengthToArray(out, radius);
  AddLengthToArray(out, center_x);
  AddLengthToArray(out, center_y);
}

/**
 * Set basic shape path, input is [type, string].
 * @param raw [type, string] array.
 */
void CSSStyleUtils::ComputeBasicShapePath(const fml::RefPtr<lepus::CArray>& raw,
                                          bool reset,
                                          fml::RefPtr<lepus::CArray>& out) {
  if (reset) {
    // Keep the array empty.
    return;
  }
  // Don't need change anything in BasicShapePath.
  // [typePath, dataString]
  out = raw;
}

/**
 *  Convert the parse result `raw` to array with platform unit `out`.
 * @param raw parsed css value
 * @param reset need reset
 * @param out output array, all units are converted to platform units
 */
void CSSStyleUtils::ComputeSuperEllipse(const fml::RefPtr<lepus::CArray>& raw,
                                        bool reset,
                                        fml::RefPtr<lepus::CArray>& out,
                                        const tasm::CssMeasureContext& context,
                                        const tasm::CSSParserConfigs& configs) {
  if (reset) {
    // Keep the array empty.
    return;
  }
  constexpr int INDEX_SUPER_ELLIPSE_TYPE = 0;
  constexpr int INDEX_SUPER_ELLIPSE_RADIUS_X = 1;
  constexpr int INDEX_SUPER_ELLIPSE_RADIUS_X_UNIT = 2;
  constexpr int INDEX_SUPER_ELLIPSE_RADIUS_Y = 3;
  constexpr int INDEX_SUPER_ELLIPSE_RADIUS_Y_UNIT = 4;
  constexpr int INDEX_SUPER_ELLIPSE_EXPONENT_X = 5;
  constexpr int INDEX_SUPER_ELLIPSE_EXPONENT_Y = 6;
  constexpr int INDEX_SUPER_ELLIPSE_CENTER_X = 7;
  constexpr int INDEX_SUPER_ELLIPSE_CENTER_X_UNIT = 8;
  constexpr int INDEX_SUPER_ELLIPSE_CENTER_Y = 9;
  constexpr int INDEX_SUPER_ELLIPSE_CENTER_Y_UNIT = 10;

  // Append type
  out->push_back(raw->get(INDEX_SUPER_ELLIPSE_TYPE));

  // Convert style length to platform length
  NLength radius_x = NLength::MakeAutoNLength();
  NLength radius_y = NLength::MakeAutoNLength();
  NLength center_x = NLength::MakeAutoNLength();
  NLength center_y = NLength::MakeAutoNLength();

  GetLengthData(radius_x, raw->get(INDEX_SUPER_ELLIPSE_RADIUS_X),
                raw->get(INDEX_SUPER_ELLIPSE_RADIUS_X_UNIT), context, configs);
  GetLengthData(radius_y, raw->get(INDEX_SUPER_ELLIPSE_RADIUS_Y),
                raw->get(INDEX_SUPER_ELLIPSE_RADIUS_Y_UNIT), context, configs);
  GetLengthData(center_x, raw->get(INDEX_SUPER_ELLIPSE_CENTER_X),
                raw->get(INDEX_SUPER_ELLIPSE_CENTER_X_UNIT), context, configs);
  GetLengthData(center_y, raw->get(INDEX_SUPER_ELLIPSE_CENTER_Y),
                raw->get(INDEX_SUPER_ELLIPSE_CENTER_Y_UNIT), context, configs);

  // re-build array, [type, rx, urx, ry, ury, ex, ey, cx, ucx, cy, ucy]
  AddLengthToArray(out, radius_x);
  AddLengthToArray(out, radius_y);
  out->push_back(raw->get(INDEX_SUPER_ELLIPSE_EXPONENT_X));
  out->push_back(raw->get(INDEX_SUPER_ELLIPSE_EXPONENT_Y));
  AddLengthToArray(out, center_x);
  AddLengthToArray(out, center_y);
}

void CSSStyleUtils::ComputeBasicShapeInset(
    const fml::RefPtr<lepus::CArray>& raw, bool reset,
    const fml::RefPtr<lepus::CArray>& dst,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  if (reset) {
    // keep the dst array empty.
    return;
  }
  constexpr int INDEX_INSET_TYPE = 0;
  dst->push_back(raw->get(INDEX_INSET_TYPE));
  NLength length = NLength::MakeAutoNLength();

  // Get inset for the four sides.
  constexpr int ARRAY_LENGTH_INSET_RECT = 8;
  for (int i = 1; i < ARRAY_LENGTH_INSET_RECT; i += 2) {
    GetLengthData(length, raw->get(i), raw->get(i + 1), context, configs);
    AddLengthToArray(dst, length);
  }
  constexpr int ARRAY_LENGTH_INSET_ROUNDED = 25;
  // raw array is arranged [type, top, unit, right, unit, bottom, unit, left,
  // unit, top-left-x, unit, top-left-y, unit, top-right-x, unit, top-right-y,
  // unit, bottom-right-x, unit, bottom-right-y, unit, bottom-left-x, unit,
  // bottom-left-y, unit]
  if (raw->size() == ARRAY_LENGTH_INSET_ROUNDED) {
    // Get <border-radius> for the four sides.
    for (int i = ARRAY_LENGTH_INSET_RECT + 1; i < ARRAY_LENGTH_INSET_ROUNDED;
         i += 2) {
      GetLengthData(length, raw->get(i), raw->get(i + 1), context, configs);
      AddLengthToArray(dst, length);
    }
  }
  constexpr int ARRAY_LENGTH_INSET_SUPER_ELLIPSE = 27;
  // raw array is arranged [type, top, unit, right, unit, bottom, unit, left,
  // unit, ex, ey, top-left-x, unit, top-left-y, unit, top-right-x, unit,
  // top-right-y, unit, bottom-right-x, unit, bottom-right-y, unit,
  // bottom-left-x, unit, bottom-left-y, unit]
  if (raw->size() == ARRAY_LENGTH_INSET_SUPER_ELLIPSE) {
    // get exponent for [ex, ey]
    dst->push_back(raw->get(ARRAY_LENGTH_INSET_RECT + 1));
    dst->push_back(raw->get(ARRAY_LENGTH_INSET_RECT + 2));
    // Get <border-radius> for the four sides.
    for (int i = ARRAY_LENGTH_INSET_RECT + 3;
         i < ARRAY_LENGTH_INSET_SUPER_ELLIPSE; i += 2) {
      GetLengthData(length, raw->get(i), raw->get(i + 1), context, configs);
      AddLengthToArray(dst, length);
    }
  }
}

bool CSSStyleUtils::IsBorderLengthLegal(std::string value) {
  return value == "thick" || value == "medium" || value == "thin" ||
         base::EndsWith(value, "px") || base::EndsWith(value, "rpx") ||
         base::EndsWith(value, "em") || base::EndsWith(value, "rem") ||
         base::EndsWith(value, "%");
}

void CSSStyleUtils::ComputeRadialGradient(
    const lepus::Value& gradient_data, const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  // Gradient shape array ref:
  // [shape, shape_size, pos_x, pos_x_value, pos_y, pos_y_value, ...]
  auto shape_arr = gradient_data.Array()->get(0).Array();
  // Convert the CSS value to platform value
  if (shape_arr->get(1).Number() ==
      static_cast<uint32_t>(starlight::RadialGradientSizeType::kLength)) {
    // Ignore the result of to length
    CSSStyleUtils::AddLengthToArray(
        shape_arr, CSSStyleUtils::ToLength(
                       tasm::CSSValue(shape_arr->get(7),
                                      static_cast<tasm::CSSValuePattern>(
                                          shape_arr->get(6).Number())),
                       context, configs)
                       .first);
    CSSStyleUtils::AddLengthToArray(
        shape_arr, CSSStyleUtils::ToLength(
                       tasm::CSSValue(shape_arr->get(9),
                                      static_cast<tasm::CSSValuePattern>(
                                          shape_arr->get(8).Number())),
                       context, configs)
                       .first);
  }
}

lepus::Value CSSStyleUtils::GetGradientArrayFromString(
    const char* gradient_def, size_t gradient_def_length,
    const tasm::CssMeasureContext& context,
    const tasm::CSSParserConfigs& configs) {
  auto parser = lynx::tasm::CSSStringParser(
      gradient_def, static_cast<uint32_t>(gradient_def_length), configs);
  auto value = parser.ParseTextColor();
  if (!value.IsArray()) {
    return lepus::Value();
  }
  const auto& defArray = value.GetValue().Array();
  auto type = defArray->get(0).Number();

  if (type ==
      static_cast<int32_t>(starlight::BackgroundImageType::kRadialGradient)) {
    const auto& gradientData = defArray->get(1);
    lynx::starlight::CSSStyleUtils::ComputeRadialGradient(gradientData, context,
                                                          configs);
  }
  return std::move(value.GetValue());
}

}  // namespace starlight
}  // namespace lynx
