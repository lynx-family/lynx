// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/unit_handler.h"

#include <cstdarg>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/parser/animation_direction_handler.h"
#include "core/renderer/css/parser/animation_fill_mode_handler.h"
#include "core/renderer/css/parser/animation_iteration_count_handler.h"
#include "core/renderer/css/parser/animation_name_handler.h"
#include "core/renderer/css/parser/animation_play_state_handler.h"
#include "core/renderer/css/parser/animation_property_handler.h"
#include "core/renderer/css/parser/animation_shorthand_handler.h"
#include "core/renderer/css/parser/aspect_ratio_handler.h"
#include "core/renderer/css/parser/auto_font_size_handler.h"
#include "core/renderer/css/parser/auto_font_size_preset_sizes_handler.h"
#include "core/renderer/css/parser/background_box_handler.h"
#include "core/renderer/css/parser/background_image_handler.h"
#include "core/renderer/css/parser/background_position_handler.h"
#include "core/renderer/css/parser/background_repeat_handler.h"
#include "core/renderer/css/parser/background_shorthand_handler.h"
#include "core/renderer/css/parser/background_size_handler.h"
#include "core/renderer/css/parser/bool_handler.h"
#include "core/renderer/css/parser/border_handler.h"
#include "core/renderer/css/parser/border_radius_handler.h"
#include "core/renderer/css/parser/border_style_handler.h"
#include "core/renderer/css/parser/border_width_handler.h"
#include "core/renderer/css/parser/clip_path_handler.h"
#include "core/renderer/css/parser/color_handler.h"
#include "core/renderer/css/parser/cursor_handler.h"
#include "core/renderer/css/parser/enum_handler.h"
#include "core/renderer/css/parser/filter_handler.h"
#include "core/renderer/css/parser/flex_flow_handler.h"
#include "core/renderer/css/parser/flex_handler.h"
#include "core/renderer/css/parser/font_length_handler.h"
#include "core/renderer/css/parser/four_sides_shorthand_handler.h"
#include "core/renderer/css/parser/gap_handler.h"
#include "core/renderer/css/parser/grid_position_handler.h"
#include "core/renderer/css/parser/grid_template_handler.h"
#include "core/renderer/css/parser/handler_defines.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/parser/list_gap_handler.h"
#include "core/renderer/css/parser/mask_shorthand_handler.h"
#include "core/renderer/css/parser/number_handler.h"
#include "core/renderer/css/parser/relative_align_handler.h"
#include "core/renderer/css/parser/shadow_handler.h"
#include "core/renderer/css/parser/string_handler.h"
#include "core/renderer/css/parser/text_decoration_handler.h"
#include "core/renderer/css/parser/text_stroke_handler.h"
#include "core/renderer/css/parser/time_handler.h"
#include "core/renderer/css/parser/timing_function_handler.h"
#include "core/renderer/css/parser/transform_handler.h"
#include "core/renderer/css/parser/transform_origin_handler.h"
#include "core/renderer/css/parser/transition_shorthand_handler.h"
#include "core/renderer/css/parser/vertical_align_handler.h"

namespace lynx {
namespace tasm {

UnitHandler& UnitHandler::Instance() {
  static base::NoDestructor<UnitHandler> instance;
  return *instance;
}

void UnitHandler::ReportError(std::string error_msg, std::string fix_suggestion,
                              CSSPropertyID key, const std::string& input) {
  base::LynxError error(error::E_CSS_PARSER, std::move(error_msg),
                        std::move(fix_suggestion), base::LynxErrorLevel::Error,
                        true);
  static const base::NoDestructor<std::string> first("css_property");
  static const base::NoDestructor<std::string> second("css_value");
  error.AddContextInfo(*first, CSSProperty::GetPropertyNameCStr(key));
  error.AddContextInfo(*second, input);
  base::ErrorStorage::GetInstance().SetError(std::move(error));
}

namespace {
void _CSSWarning(const char* fmt, va_list args) {
  auto error_msg = base::FormatStringWithVaList(fmt, args);
  LynxWarning(false, error::E_CSS_PARSER, error_msg);
}
}  // namespace

bool UnitHandler::CSSWarningUnconditional(const char* fmt...) {
  va_list args;
  va_start(args, fmt);
  _CSSWarning(fmt, args);
  va_end(args);
  return false;
}

/*
 * if a function calls CSSWarning() but is supposed to return immediately, do
 * things below like:
 * if(!CSSWarning(...)) {return false;}
 *  // then do something
 */
bool UnitHandler::CSSWarning(bool expression, bool enable_css_strict_mode,
                             const char* fmt, ...) {
  if (UNLIKELY(!(expression))) {
    if (enable_css_strict_mode) {
      va_list args;
      va_start(args, fmt);
      _CSSWarning(fmt, args);
      va_end(args);
    }
    return false;
  }
  return true;
}

bool UnitHandler::CSSUnreachable(bool enable_css_strict_mode, const char* fmt,
                                 ...) {
  if (UNLIKELY(enable_css_strict_mode)) {
    va_list args;
    va_start(args, fmt);
    _CSSWarning(fmt, args);
    va_end(args);
  }
  return false;
}

bool UnitHandler::CSSMethodUnreachable(bool enable_css_strict_mode) {
  CSSUnreachable(enable_css_strict_mode, CANNOT_REACH_METHOD);
  return false;
}

bool UnitHandler::ProcessCSSValue(const CSSPropertyID key,
                                  const tasm::CSSValue& input, StyleMap& output,
                                  const CSSParserConfigs& configs) {
  if (input.IsVariable()) {
    output[key] = input;
    return true;
  }
  return UnitHandler::Process(key, input.GetValue(), output, configs);
}

bool UnitHandler::Process(const CSSPropertyID key, const lepus::Value& input,
                          StyleMap& output, const CSSParserConfigs& configs) {
  if (key <= CSSPropertyID::kPropertyStart ||
      key >= CSSPropertyID::kPropertyEnd) {
    LOGE("[UnitHandler] illegal css key:" << key);
    CSSUnreachable(configs.enable_css_strict_mode,
                   "[UnitHandler] illegal css key:%d", key);
    return false;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UnitHandler::Process",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations(
                    "property_name", CSSProperty::GetPropertyName(key).str());
              });
  auto maybe_handler = Instance().interceptors_[key];
  if (!maybe_handler) {
    output[key] = CSSValue(input);
    return true;
  }

  if (output.empty()) {
    // If target map is empty, we have the opportunity to reserve memory
    // for it. This will optimize the case that a shorthand inline style
    // is set by render functions.
    if (auto expand = CSSProperty::GetShorthandExpand(key); expand > 0) {
      output.reserve(expand + kCSSStyleMapFuzzyAllocationSize);
    }
  }

  if (!maybe_handler(key, input, output, configs)) {
    if (!configs.remove_css_parser_log) {
      std::ostringstream output_value;
      input.PrintValue(output_value, false, false);
      LOGE("[UnitHandler] css:" << CSSProperty::GetPropertyName(key).c_str()
                                << " has invalid value " << output_value.str()
                                << " !!! It has be ignored.");
    }
    return false;
  }
  return true;
}

StyleMap UnitHandler::Process(const CSSPropertyID key,
                              const lepus::Value& input,
                              const CSSParserConfigs& configs) {
  StyleMap ret(CSSProperty::GetShorthandExpand(key));
  Process(key, input, ret, configs);
  return ret;
}

UnitHandler::UnitHandler() {
  // TODO(liyanbo): must at first position. other will replace pre define.
  StringHandler::Register(interceptors_);
  AnimationDirectionHandler::Register(interceptors_);
  AnimationFillModeHandler::Register(interceptors_);
  AnimationPlayStateHandler::Register(interceptors_);
  AnimationPropertyHandler::Register(interceptors_);
  AnimationNameHandler::Register(interceptors_);
  AnimationShorthandHandler::Register(interceptors_);
  AspectRatioHandler::Register(interceptors_);
  BoolHandler::Register(interceptors_);
  ColorHandler::Register(interceptors_);
  BorderHandler::Register(interceptors_);
  TextStrokeHandler::Register(interceptors_);
  BorderStyleHandler::Register(interceptors_);
  BorderWidthHandler::Register(interceptors_);
  EnumHandler::Register(interceptors_);
  FlexFlowHandler::Register(interceptors_);
  FlexHandler::Register(interceptors_);
  FontLengthHandler::Register(interceptors_);
  FourSidesShorthandHandler::Register(interceptors_);
  GridPositionHandler::Register(interceptors_);
  GridTemplateHandler::Register(interceptors_);
  LengthHandler::Register(interceptors_);
  NumberHandler::Register(interceptors_);
  AnimIterCountHandler::Register(interceptors_);
  ShadowHandler::Register(interceptors_);
  TimeHandler::Register(interceptors_);
  TimingFunctionHandler::Register(interceptors_);
  TransformHandler::Register(interceptors_);
  TransformOriginHandler::Register(interceptors_);
  TransitionShorthandHandler::Register(interceptors_);
  TextDecorationHandler::Register(interceptors_);
  BorderRadiusHandler::Register(interceptors_);
  BackgroundShorthandHandler::Register(interceptors_);
  BackgroundBoxHandler::Register(interceptors_);
  BackgroundImageHandler::Register(interceptors_);
  BackgroundPositionHandler::Register(interceptors_);
  BackgroundRepeatHandler::Register(interceptors_);
  BackgroundSizeHandler::Register(interceptors_);
  MaskShorthandHandler::Register(interceptors_);
  FilterHandler::Register(interceptors_);
  VerticalAlignHandler::Register(interceptors_);
  RelativeAlignHandler::Register(interceptors_);
  ListGapHandler::Register(interceptors_);
  CursorHandler::Register(interceptors_);
  ClipPathHandler::Register(interceptors_);
  AutoFontSizeHandler::Register(interceptors_);
  AutoFontSizePresetSizesHandler::Register(interceptors_);
  GapHandler::Register(interceptors_);
}

}  // namespace tasm
}  // namespace lynx
