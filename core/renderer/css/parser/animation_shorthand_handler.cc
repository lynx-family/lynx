// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/animation_shorthand_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {
namespace AnimationShorthandHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  bool single = std::strchr(parser.content(), ',') == nullptr;
  // [name, duration, delay, timing, count, direction, fill_mode, play_state]
  lepus::Value arr[8];
  if (!parser.ParseAnimation(single, arr)) {
    return false;
  }
  // shorthand -> longhand
  // [name, duration, delay, timing, count, direction, fill_mode, play_state]
  if (key == kPropertyIDAnimation) {
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationName, arr[0],
        single ? CSSValuePattern::STRING : CSSValuePattern::ARRAY);
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationDuration, arr[1],
        single ? CSSValuePattern::NUMBER : CSSValuePattern::ARRAY);
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationDelay, arr[2],
        single ? CSSValuePattern::NUMBER : CSSValuePattern::ARRAY);
    output.emplace_or_assign(CSSPropertyID::kPropertyIDAnimationTimingFunction,
                             arr[3].Array());
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationIterationCount, arr[4],
        single ? CSSValuePattern::NUMBER : CSSValuePattern::ARRAY);
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationDirection, arr[5],
        single ? CSSValuePattern::ENUM : CSSValuePattern::ARRAY);
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationFillMode, arr[6],
        single ? CSSValuePattern::ENUM : CSSValuePattern::ARRAY);
    output.emplace_or_assign(
        CSSPropertyID::kPropertyIDAnimationPlayState, arr[7],
        single ? CSSValuePattern::ENUM : CSSValuePattern::ARRAY);
  } else {
    // [name, duration, delay, timing, count, direction, fill_mode, play_state]
    static const CSSPropertyID keys[8] = {
        kPropertyIDAnimationName,           kPropertyIDAnimationDuration,
        kPropertyIDAnimationDelay,          kPropertyIDAnimationTimingFunction,
        kPropertyIDAnimationIterationCount, kPropertyIDAnimationDirection,
        kPropertyIDAnimationFillMode,       kPropertyIDAnimationPlayState};
    auto map = lepus::Dictionary::Create();
    for (size_t i = 0; i < 8; i++) {
      map->SetValue(std::to_string(keys[i]), arr[i]);
    }
    output.emplace_or_assign(key, lepus::Value(std::move(map)),
                             CSSValuePattern::MAP);
  }
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDAnimation] = &Handle;
  array[kPropertyIDEnterTransitionName] = &Handle;
  array[kPropertyIDExitTransitionName] = &Handle;
  array[kPropertyIDPauseTransitionName] = &Handle;
  array[kPropertyIDResumeTransitionName] = &Handle;
}

}  // namespace AnimationShorthandHandler
}  // namespace tasm
}  // namespace lynx
