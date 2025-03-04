// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/grid_template_handler.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace GridTemplateHandler {

// "repeat()" string size.
namespace {
constexpr size_t kRepeatFunMinSize = 8;
constexpr size_t kMinmaxFunMinSize = 8;
constexpr const char* kValueRepeat = "repeat";
constexpr const char* kValueMinmax = "minmax";
constexpr const char* kValueErrorMessage =
    "value must be a string or percentage array:%d";

bool ParserTrackListValue(const std::string& len_arr_str,
                          fml::RefPtr<lepus::CArray> array,
                          const CSSParserConfigs& configs) {
  std::vector<std::string> arr_values;
  base::SplitStringBySpaceOutOfBrackets(len_arr_str, arr_values);

  const auto& ParserLengthValue =
      [array, configs](const std::string& value_str) -> bool {
    tasm::CSSValue css_value;
    lepus::Value lepus_value(value_str);
    if (!LengthHandler::Process(lepus_value, css_value, configs)) {
      return false;
    }

    array->emplace_back(std::move(css_value.GetValue()));
    array->emplace_back(static_cast<int32_t>(css_value.GetPattern()));
    return true;
  };

  for (const std::string& value_str : arr_values) {
    if (value_str.empty()) {
      continue;
    }

    const std::string::size_type minmax_pos = value_str.find(kValueMinmax);
    // resolve minmax function.
    if (minmax_pos != std::string::npos) {
      const std::string& content_str = value_str.substr(
          kMinmaxFunMinSize - 1, value_str.size() - kMinmaxFunMinSize);
      std::vector<std::string> content_arr;
      base::SplitString(content_str, ',', content_arr);
      // Insert two additional lepus::Value, representing that the next two
      // Length denotes the two parameters in minmax() function.
      array->emplace_back(static_cast<int32_t>(CSSFunctionType::MINMAX));
      array->emplace_back(static_cast<int32_t>(CSSValuePattern::ENUM));

      if (content_arr.size() != 2 || !ParserLengthValue(content_arr[0]) ||
          !ParserLengthValue(content_arr[1])) {
        return false;
      }
    } else {
      if (!ParserLengthValue(value_str)) {
        return false;
      }
    }
  }
  return true;
}

// parm format:"repeat(size,content);"
bool ResolveRepeatFunc(const std::string& repeat_func,
                       fml::RefPtr<lepus::CArray> array,
                       const CSSParserConfigs& configs) {
  if (!base::BeginsWith(repeat_func, kValueRepeat)) {
    return false;
  }

  const std::string& content_str = repeat_func.substr(
      kRepeatFunMinSize - 1, repeat_func.size() - kRepeatFunMinSize);
  const std::string::size_type comma_pos = content_str.find(',');
  if (comma_pos == std::string::npos) {
    return false;
  }
  const std::string& repeat_size_string = content_str.substr(0, comma_pos);
  const std::string& track_list_string = content_str.substr(comma_pos + 1);
  const int32_t repeat_size = std::max(atoi(repeat_size_string.c_str()), 0);
  for (int32_t idx = 0; idx < repeat_size; ++idx) {
    if (!ParserTrackListValue(track_list_string, array, configs)) {
      return false;
    }
  }

  return true;
}
}  // namespace

HANDLER_IMPL() {
  if (!(input.IsString())) {
    return false;
  }

  auto array = lepus::CArray::Create();
  const auto& value_str = input.StdString();
  std::string::size_type value_str_idx = 0;
  std::string::size_type value_str_size = value_str.size();
  while (value_str_idx < value_str_size) {
    std::string::size_type repeat_pos =
        value_str.find(kValueRepeat, value_str_idx);
    std::string::size_type length_end =
        repeat_pos == std::string::npos ? value_str_size : repeat_pos;

    CSS_HANDLER_FAIL_IF_NOT(
        ParserTrackListValue(
            value_str.substr(value_str_idx, length_end - value_str_idx), array,
            configs),
        configs.enable_css_strict_mode, kValueErrorMessage, key)

    if (repeat_pos != std::string::npos) {
      std::string::size_type repeat_end_pos =
          repeat_pos + kRepeatFunMinSize - 1;
      if (repeat_end_pos >= value_str_size) {
        return false;
      }
      int32_t parenthesis_balance = 1;
      for (; repeat_end_pos < value_str_size; ++repeat_end_pos) {
        char current_char = value_str[repeat_end_pos];
        if (current_char == '(') {
          parenthesis_balance++;
        } else if (current_char == ')') {
          parenthesis_balance--;
          if (parenthesis_balance == 0) {
            break;
          }
        }
      }

      // The number of ")" should be matched with the number of "(".
      if (parenthesis_balance != 0) {
        return false;
      }

      CSS_HANDLER_FAIL_IF_NOT(
          ResolveRepeatFunc(
              value_str.substr(repeat_pos, repeat_end_pos - length_end + 1),
              array, configs),
          configs.enable_css_strict_mode, kValueErrorMessage, key)
      value_str_idx = repeat_end_pos + 1;
    } else {
      value_str_idx = length_end;
    }
  }

  output.emplace_or_assign(key, std::move(array));
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDGridTemplateColumns] = &Handle;
  array[kPropertyIDGridTemplateRows] = &Handle;
  array[kPropertyIDGridAutoColumns] = &Handle;
  array[kPropertyIDGridAutoRows] = &Handle;
}

}  // namespace GridTemplateHandler
}  // namespace tasm
}  // namespace lynx
