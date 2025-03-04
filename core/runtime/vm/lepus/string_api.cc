// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/string_api.h"

#include <utility>

#include "base/include/string/string_utils.h"
#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/vm_context.h"

#define CAPTURE_COUNT_MAX 255
extern "C" {
#include "quickjs/include/cutils.h"
#include "quickjs/include/libregexp.h"
}

namespace lynx {
namespace lepus {
int GetRegExpFlags(const std::string& flags) {
  int re_flags = 0;
  int mask = 0;
  for (char flag : flags) {
    switch (flag) {
      case 'g':
        mask = LRE_FLAG_GLOBAL;
        break;
      case 'i':
        mask = LRE_FLAG_IGNORECASE;
        break;
      case 'm':
        mask = LRE_FLAG_MULTILINE;
        break;
      case 's':
        mask = LRE_FLAG_DOTALL;
        break;
      case 'u':
        mask = LRE_FLAG_UTF16;
        break;
      case 'y':
        mask = LRE_FLAG_STICKY;
        break;
      default:
        break;
    }
    re_flags |= mask;
  }
  return re_flags;
}

std::pair<size_t, bool> GetUnicodeFromUft8(const char* input, size_t input_len,
                                           uint16_t* output,
                                           size_t output_length) {
  DCHECK(output_length >= input_len);
  const uint8_t *p, *p_end, *p_start;

  // check is no contain unicode
  const uint8_t *p_check, *p_check_end, *p_check_start;
  p_check_start = reinterpret_cast<const uint8_t*>(input);
  p_check = p_check_start;
  p_check_end = p_check_start + input_len;
  while (p_check < p_check_end && *p_check < 128) {
    p_check++;
  }

  if (p_check == p_check_end) {
    memcpy(output, input, input_len);
    return {input_len, false};
  } else {
    p_start = reinterpret_cast<const uint8_t*>(input);
    p = p_start;
    p_end = p_start + input_len;
    size_t unicode_len = 0;
    for (size_t i = 0; i < input_len && p < p_end; i++) {
      output[i] = unicode_from_utf8(p, UTF8_CHAR_LEN_MAX, &p);
      unicode_len++;
    }
    return {unicode_len, true};
  }
}

// ref
// https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Global_Objects/String/replace#%E4%BD%BF%E7%94%A8%E5%AD%97%E7%AC%A6%E4%B8%B2%E4%BD%9C%E4%B8%BA%E5%8F%82%E6%95%B0
std::string GetReplaceStr(const std::string& data,
                          const std::string& need_to_replace_str,
                          const std::string& replace_to_str, int32_t position) {
  std::string string_to_replace = "";
  for (size_t i = 0; i < replace_to_str.length();) {
    char ch = replace_to_str[i];
    if (ch == '$' && i < replace_to_str.length() - 1) {
      switch (replace_to_str[i + 1]) {
        case '$': {
          // char '$'
          i += 2;
          string_to_replace.push_back('$');
          break;
        }
        case '&': {
          // string need to be replaced
          i += 2;
          string_to_replace.insert(string_to_replace.end(),
                                   need_to_replace_str.begin(),
                                   need_to_replace_str.end());
          break;
        }
        case '`': {
          // left content of the matched substring
          i += 2;
          std::string match_substr_before = data.substr(0, position);
          string_to_replace.insert(string_to_replace.end(),
                                   match_substr_before.begin(),
                                   match_substr_before.end());
          break;
        }
        case '\'': {
          // right content of the matched substring
          i += 2;
          std::string match_substr_after = data.substr(position + 1);
          string_to_replace.insert(string_to_replace.end(),
                                   match_substr_after.begin(),
                                   match_substr_after.end());
          break;
        }
        default: {
          string_to_replace.push_back(ch);
          i++;
          break;
        }
      }
    } else {
      string_to_replace.push_back(ch);
      i++;
    }
  }
  return string_to_replace;
}

static std::string GetReplaceStr(const std::string& param2_str,
                                 const CArray& array_global,
                                 const int& match_index,
                                 const base::String& input, uint8_t* bc,
                                 const bool& global_mode) {
  std::string string_to_replace;
  auto find_match_array = array_global.get(match_index).Array();
  for (size_t i = 0; i < param2_str.size();) {
    char ch = param2_str[i];
    if (ch == '$' && i < param2_str.size() - 1) {
      switch (param2_str[i + 1]) {
        case '$': {
          i += 2;
          string_to_replace.push_back('$');
          break;
        }
        case '&': {
          i += 2;
          const std::string& match_substr =
              find_match_array->get(1).StdString();
          string_to_replace.insert(string_to_replace.end(),
                                   match_substr.begin(), match_substr.end());
          break;
        }
        case '`': {
          i += 2;
          int ret;
          int find_match_inner = 0;

          size_t start_search_index = 0;
          base::InlineVector<uint16_t, 512> str_c;
          str_c.resize<false>(input.length());
          const auto [unicode_len, has_unicode] = GetUnicodeFromUft8(
              input.c_str(), input.length(), str_c.data(), str_c.size());
          int shift = has_unicode ? 1 : 0;
          while (start_search_index <= unicode_len) {
            uint8_t* capture[CAPTURE_COUNT_MAX * 2];
            ret =
                lre_exec(capture, bc, reinterpret_cast<uint8_t*>(str_c.data()),
                         static_cast<int>(start_search_index),
                         static_cast<int>(unicode_len), shift, nullptr);
            if (ret == 0 || ret == -1) {
              break;
            }
            DCHECK(capture[0] && capture[1]);

            size_t match_start =
                (capture[0] - reinterpret_cast<uint8_t*>(str_c.data())) >>
                shift;
            size_t match_end =
                (capture[1] - reinterpret_cast<uint8_t*>(str_c.data())) >>
                shift;
            std::string str_to_replace;

            if (find_match_inner == match_index) {
              std::string match_substr_before;
              if (has_unicode) {
                size_t match_start_C = base::UTF8IndexToCIndex(
                    input.c_str(), input.length(), match_start);
                match_substr_before = input.str().substr(0, match_start_C);
              } else {
                match_substr_before = input.str().substr(0, match_start);
              }
              string_to_replace.insert(string_to_replace.end(),
                                       match_substr_before.begin(),
                                       match_substr_before.end());
              break;
            } else {
              find_match_inner++;
              if (global_mode) {
                start_search_index = match_end;
              } else {
                start_search_index = unicode_len + 1;
              }
            }
          }
          break;
        }
        case '\'': {
          i += 2;
          int ret;
          size_t start_search_index = 0;
          int find_match_inner = 0;
          base::InlineVector<uint16_t, 512> str_c;
          str_c.resize<false>(input.length());
          const auto [unicode_len, has_unicode] = GetUnicodeFromUft8(
              input.c_str(), input.length(), str_c.data(), str_c.size());
          int shift = has_unicode ? 1 : 0;
          while (start_search_index <= unicode_len) {
            uint8_t* capture[CAPTURE_COUNT_MAX * 2];
            ret =
                lre_exec(capture, bc, reinterpret_cast<uint8_t*>(str_c.data()),
                         static_cast<int>(start_search_index),
                         static_cast<int>(unicode_len), shift, nullptr);
            if (ret == 0 || ret == -1) {
              break;
            }
            DCHECK(capture[0] && capture[1]);

            size_t match_end =
                (capture[1] - reinterpret_cast<uint8_t*>(str_c.data())) >>
                shift;
            std::string str_to_replace;

            if (find_match_inner == match_index) {
              std::string match_substr_after;
              if (has_unicode) {
                size_t match_end_C = base::UTF8IndexToCIndex(
                    input.c_str(), input.length(), match_end);
                match_substr_after = input.str().substr(match_end_C);
              } else {
                match_substr_after = input.str().substr(match_end);
              }
              string_to_replace.insert(string_to_replace.end(),
                                       match_substr_after.begin(),
                                       match_substr_after.end());
              break;
            } else {
              find_match_inner++;
              if (global_mode) {
                start_search_index = match_end;
              } else {
                start_search_index = unicode_len + 1;
              }
            }
          }
          break;
        }
        default: {
          int param_num = 0;
          size_t j = i + 1;
          for (; j < param2_str.size(); j++) {
            if (param2_str[j] >= '0' && param2_str[j] <= '9') {
              param_num = param_num * 10 + (param2_str[j] - '0');
            } else {
              break;
            }
          }
          i = j;

          const std::string& parentheses_str =
              find_match_array->get(1 + param_num * 3).StdString();
          string_to_replace.insert(string_to_replace.end(),
                                   parentheses_str.begin(),
                                   parentheses_str.end());
          break;
        }
      }
    } else {
      string_to_replace.push_back(ch);
      i++;
    }
  }
  return string_to_replace;
}

static void GetReplaceResult(const std::string& str_to_replace,
                             std::string& result, bool has_unicode,
                             bool& str_to_replace_has_unicode,
                             size_t& str_to_replace_unicode_len,
                             size_t match_start, size_t match_end) {
  base::InlineVector<uint16_t, 512> str_to_replace_ctr;
  str_to_replace_ctr.resize<false>(str_to_replace.length());
  std::tie(str_to_replace_unicode_len, str_to_replace_has_unicode) =
      GetUnicodeFromUft8(str_to_replace.c_str(), str_to_replace.length(),
                         str_to_replace_ctr.data(), str_to_replace_ctr.size());
  if (has_unicode) {
    size_t match_start_C =
        base::UTF8IndexToCIndex(result.c_str(), result.length(), match_start);
    size_t match_end_C =
        base::UTF8IndexToCIndex(result.c_str(), result.length(), match_end);
    result = result.replace(match_start_C, match_end_C - match_start_C,
                            str_to_replace.c_str(), str_to_replace.length());
  } else {
    result = result.replace(match_start, match_end - match_start,
                            str_to_replace.c_str(), str_to_replace.length());
  }
}

static void GetRegExecuteResult(const int& capture_count, uint8_t** capture,
                                const int& shift, const std::string& result,
                                uint16_t* str_c, size_t& match_start,
                                size_t& match_end, const bool& has_unicode,
                                CArray& array_global) {
  auto array_data = CArray::Create();
  array_data->reserve(3 * capture_count + 1);
  array_data->emplace_back(result);
  for (int i = 0; i < capture_count; i++) {
    if (capture[2 * i] == nullptr || capture[2 * i + 1] == nullptr) continue;
    size_t start =
        (capture[2 * i] - reinterpret_cast<uint8_t*>(str_c)) >> shift;
    size_t end =
        (capture[2 * i + 1] - reinterpret_cast<uint8_t*>(str_c)) >> shift;
    if (i == 0) {
      match_start = start;
      match_end = end;
    }

    if (has_unicode) {
      size_t match_start_C =
          base::UTF8IndexToCIndex(result.c_str(), result.length(), start);
      size_t match_end_C =
          base::UTF8IndexToCIndex(result.c_str(), result.length(), end);
      array_data->emplace_back(
          result.substr(match_start_C, match_end_C - match_start_C));
    } else {
      array_data->emplace_back(result.substr(start, end - start));
    }
    array_data->emplace_back(static_cast<int32_t>(start));
    array_data->emplace_back(static_cast<int32_t>(end));
  }
  array_global.emplace_back(std::move(array_data));
}

static Value Search(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(context->GetParam(params_count - 1)->IsString());
  const auto& str = context->GetParam(params_count - 1)->StdString();

  fml::RefPtr<lepus::RegExp> reg_exp;
  if (params_count != 1) {
    DCHECK(params_count == 2);
    if (context->GetParam(0)->IsRegExp()) {
      reg_exp = context->GetParam(0)->RegExp();
    } else {
      DCHECK(context->GetParam(0)->IsString());
      reg_exp = RegExp::Create(context->GetParam(0)->String());
    }
  } else {
    // no param
    return Value(static_cast<int64_t>(0));
  }

  const char* pattern = reg_exp->get_pattern().c_str();
  const std::string& flags = reg_exp->get_flags().str();

  // search function
  uint8_t* bc;
  char error_msg[64];
  int len, ret;
  int re_flags = GetRegExpFlags(flags);
  bc = lre_compile(&len, error_msg, sizeof(error_msg), pattern, strlen(pattern),
                   re_flags, nullptr);

  if (bc == nullptr) {
    context->ReportError("SyntaxError: Invalid regular expression: /" +
                         reg_exp->get_pattern().str() + "/:" + error_msg);
    return Value();
  }

  base::InlineVector<uint16_t, 512> str_c;
  str_c.resize<false>(str.length());
  const auto [unicode_len, has_unicode] =
      GetUnicodeFromUft8(str.c_str(), str.length(), str_c.data(), str_c.size());
  int shift = has_unicode ? 1 : 0;
  uint8_t* capture[CAPTURE_COUNT_MAX * 2];
  ret = lre_exec(capture, bc, reinterpret_cast<uint8_t*>(str_c.data()), 0,
                 static_cast<int>(unicode_len), shift, nullptr);
  // free bc
  free(bc);

  int64_t start = -1;
  if (ret == 1 && capture[0]) {
    start = (capture[0] - reinterpret_cast<uint8_t*>(str_c.data())) >> shift;
  }

  return Value(start);
}

static Value Trim(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1);
  DCHECK(context->GetParam(0)->IsString());
  // Check if trim-able to avoid string copy.
  const auto& ori_str = context->GetParam(0)->StdString();
  auto left_pos = ori_str.find_first_not_of(" ");
  if (left_pos == std::string::npos) {
    // All spaces or empty string
    return Value(base::String());
  }

  auto right_pos = ori_str.find_last_not_of(" ");
  if (left_pos == 0 && right_pos == ori_str.length() - 1) {
    // No need to trim.
    return *context->GetParam(0);
  }

  // String needs copy
  auto str = ori_str;
  str.erase(right_pos + 1);  // trim right first
  str.erase(0, left_pos);
  return Value(std::move(str));
}

static Value CharAt(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(context->GetParam(params_count - 1)->IsString());
  const auto& str = context->GetParam(params_count - 1)->StdString();
  size_t pos = 0;
  if (params_count != 1) {
    DCHECK(params_count == 2);
    DCHECK(context->GetParam(0)->IsNumber());
    pos = static_cast<size_t>(
        static_cast<int64_t>(context->GetParam(0)->Number()));
  }
  if (pos >= 0 && pos < str.length())
    return Value(str.substr(pos, 1));
  else
    return Value(base::String());
}

static Value Match(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(context->GetParam(params_count - 1)->IsString());
  Value result = Value(CArray::Create());
  auto result_array = result.Array();
  result_array->SetIsMatchResult();
  auto str = context->GetParam(params_count - 1)->String();

  // no params
  if (params_count == 1) {
    result_array->emplace_back(base::String());
    result_array->emplace_back(0);
    result_array->emplace_back(str);
    result_array->push_back_default();
    return result;
  }

  DCHECK(params_count == 2);

  std::string pattern;
  std::string flags;
  Value* param;
  int re_flags = 0;

  // handle param:
  param = context->GetParam(0);
  if (param->IsRegExp()) {
    auto reg_exp = context->GetParam(0)->RegExp();
    pattern = reg_exp->get_pattern().str();
    flags = reg_exp->get_flags().str();
    re_flags = GetRegExpFlags(flags);
  } else {
    if (param->IsString()) {
      pattern = param->StdString();
    } else if (param->IsNil()) {
      pattern = "null";
    } else if (param->IsNumber()) {
      switch (param->Type()) {
        case Value_Int64:
        case Value_UInt64: {
          pattern = std::to_string(param->Int64());
          break;
        }
        case Value_Int32:
        case Value_UInt32: {
          pattern = std::to_string(param->Int32());
          break;
        }
        case Value_Double: {
          pattern = std::to_string(param->Number());
        }
        default:
          break;
      }
    }
  }

  // match function
  uint8_t* bc;
  char error_msg[64];
  int len, ret;
  bc = lre_compile(&len, error_msg, sizeof(error_msg), pattern.c_str(),
                   pattern.length(), re_flags, nullptr);

  if (bc == nullptr) {
    context->ReportError("SyntaxError: Invalid regular expression: /" +
                         pattern + "/: " + error_msg);
    return Value();
  }

  bool global_mode = false;
  if (flags.find('g') != std::string::npos) {
    global_mode = true;
  }

  base::InlineVector<uint16_t, 512> str_c;
  str_c.resize<false>(str.length());
  const auto [unicode_len, has_unicode] =
      GetUnicodeFromUft8(str.c_str(), str.length(), str_c.data(), str_c.size());
  int shift = has_unicode ? 1 : 0;

  size_t start_search_index = 0;
  int capture_count = -1;
  int match_num = 0;
  while (start_search_index <= unicode_len) {
    uint8_t* capture[CAPTURE_COUNT_MAX * 2];
    ret = lre_exec(capture, bc, reinterpret_cast<uint8_t*>(str_c.data()),
                   static_cast<int>(start_search_index),
                   static_cast<int>(unicode_len), shift, nullptr);
    if (ret == 0 || ret == -1) {
      if (match_num == 0) {
        result = Value();
        result_array = result.Array();
      }
      break;
    }
    DCHECK(capture[0] && capture[1]);
    if (!global_mode) {
      capture_count = lre_get_capture_count(bc);
    }

    size_t match_start = 0;
    size_t match_end = 0;
    std::string substr;
    if (global_mode) {
      match_start =
          (capture[0] - reinterpret_cast<uint8_t*>(str_c.data())) >> shift;
      match_end =
          (capture[1] - reinterpret_cast<uint8_t*>(str_c.data())) >> shift;
      if (has_unicode) {
        size_t match_start_C =
            base::UTF8IndexToCIndex(str.c_str(), str.length(), match_start);
        size_t match_end_C =
            base::UTF8IndexToCIndex(str.c_str(), str.length(), match_end);
        substr = str.str().substr(match_start_C, match_end_C - match_start_C);
      } else {
        substr = str.str().substr(match_start, match_end - match_start);
      }
      result_array->emplace_back(substr);
    } else {
      for (int i = 0; i < capture_count; i++) {
        if (!capture[2 * i] || !capture[2 * i + 1]) {
          // console.log('https'.match(/http(s)??/));
          result_array->push_back_default();
          continue;
        }
        size_t start =
            (capture[2 * i] - reinterpret_cast<uint8_t*>(str_c.data())) >>
            shift;
        size_t end =
            (capture[2 * i + 1] - reinterpret_cast<uint8_t*>(str_c.data())) >>
            shift;
        if (i == 0) {
          match_start = start;
          match_end = end;
        }

        if (has_unicode) {
          size_t match_start_C =
              base::UTF8IndexToCIndex(str.c_str(), str.length(), start);
          size_t match_end_C =
              base::UTF8IndexToCIndex(str.c_str(), str.length(), end);
          substr = str.str().substr(match_start_C, match_end_C - match_start_C);
        } else {
          substr = str.str().substr(start, end - start);
        }
        result_array->emplace_back(substr);
      }
      result_array->emplace_back(static_cast<int32_t>(match_start));
      result_array->emplace_back(str);
      BASE_STATIC_STRING_DECL(kUndefined, "undefined");
      result_array->emplace_back(kUndefined);
    }
    if (global_mode) {
      start_search_index = match_end;
    } else {
      start_search_index = unicode_len + 1;
    }
    match_num++;
  }

  if (global_mode) {
    result_array->push_back_default();
    result_array->push_back_default();
    result_array->push_back_default();
  }
  free(bc);
  return result;
}

static Value Replace(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(context->GetParam(params_count - 1)->IsString());
  auto str = context->GetParam(params_count - 1)->String();
  std::string result = str.str();
  if (params_count != 1) {
    DCHECK(params_count == 3);
  }

  Value* param1 = context->GetParam(0);
  DCHECK(param1->IsRegExp() || param1->IsString());

  // Prepare param2
  Value* param2 = context->GetParam(1);
  const std::string* param2_str_ptr;
  switch (param2->Type()) {
    case Value_String: {
      param2_str_ptr = &param2->StdString();
      break;
    }
    case Value_Nil: {
      BASE_STATIC_STRING_DECL(kNull, "null");
      param2_str_ptr = &kNull.str();
      break;
    }
    case Value_Undefined: {
      BASE_STATIC_STRING_DECL(kUndefined, "undefined");
      param2_str_ptr = &kUndefined.str();
      break;
    }
    default: {
      // Default constructed base::String' str() always points to a static alive
      // std::string.
      param2_str_ptr = &base::String().str();
      break;
    }
  }
  const std::string& param2_str =
      *param2_str_ptr;  // Do not copy param2 string.

  if (param1->IsString()) {
    // if the pattern is a string, just replace the first matched substring
    const std::string& need_to_replace = param1->StdString();
    if (!param2->IsClosure()) {
      std::string str_to_replace = "";
      auto position = result.find(need_to_replace);
      if (position != std::string::npos) {
        if (param2_str.find('$') == std::string::npos) {
          str_to_replace = param2_str;
        } else {
          str_to_replace = GetReplaceStr(result, need_to_replace, param2_str,
                                         static_cast<int32_t>(position));
        }

        if (str_to_replace == "") {
          result.erase(position, need_to_replace.length());
        } else {
          result.replace(position, need_to_replace.length(), str_to_replace, 0,
                         str_to_replace.length());
        }
      }
    }
  } else if (param1->IsRegExp()) {
    // if the pattern is a reg exp, use lre_compile and lre_exe function to
    // get the result
    auto param1_regex = param1->RegExp();
    const auto& pattern = param1_regex->get_pattern();
    const auto& flags = param1_regex->get_flags().str();
    auto re_flags = GetRegExpFlags(flags);

    uint8_t* bc;
    char error_msg[64];
    int len, ret;
    bc = lre_compile(&len, error_msg, sizeof(error_msg), pattern.c_str(),
                     pattern.length(), re_flags, nullptr);

    if (bc == nullptr) {
      context->ReportError("SyntaxError: Invalid regular expression: / " +
                           pattern.str() + "/: " + error_msg);
      return Value();
    }

    size_t start_search_index = 0;
    size_t input_len = result.length();
    int find_match = 0;

    // array_global:
    // 0: whole string
    // for every match:
    // 0: match str, 1: match_start_index, 2: match_end_index.
    // for every parentheses match:
    // 0: match str, 1: match_start_index, 2: match_end_index.ß
    auto array_global_ptr = CArray::Create();
    base::InlineVector<uint16_t, 512> str_c;

    bool global_mode = flags.find('g') != std::string::npos;
    while (start_search_index <= input_len && input_len > 0) {
      str_c.resize<false>(result.length());
      const auto [unicode_len, has_unicode] = GetUnicodeFromUft8(
          result.c_str(), result.length(), str_c.data(), str_c.size());
      int shift = has_unicode ? 1 : 0;
      uint8_t* capture[CAPTURE_COUNT_MAX * 2];
      ret = lre_exec(capture, bc, reinterpret_cast<uint8_t*>(str_c.data()),
                     static_cast<int>(start_search_index),
                     static_cast<int>(unicode_len), shift, nullptr);
      if (ret == 0 || ret == -1) {
        break;
      }
      DCHECK(capture[0] && capture[1]);

      size_t match_start = 0;
      size_t match_end = 0;
      std::string str_to_replace;
      bool str_to_replace_has_unicode = false;
      size_t str_to_replace_unicode_len = 0;

      if (!param2->IsClosure() && param2_str.find('$') == std::string::npos) {
        match_start =
            (capture[0] - reinterpret_cast<uint8_t*>(str_c.data())) >> shift;
        match_end =
            (capture[1] - reinterpret_cast<uint8_t*>(str_c.data())) >> shift;
        str_to_replace = param2_str;
        str_to_replace_unicode_len = str_to_replace.length();
        GetReplaceResult(str_to_replace, result, has_unicode,
                         str_to_replace_has_unicode, str_to_replace_unicode_len,
                         match_start, match_end);
      } else {
        int capture_count = lre_get_capture_count(bc);
        GetRegExecuteResult(capture_count, capture, shift, result, str_c.data(),
                            match_start, match_end, has_unicode,
                            *array_global_ptr);

        if (param2->IsClosure()) {
          Value* call_function = param2;
          int param_len = -1;
          Value* this_obj = context->GetParam(params_count - 1);
          Value* match = this_obj + (++param_len);
          *match =
              Value(array_global_ptr->get(find_match).Array()->get(1).String());
          size_t parentheses_match_size =
              (array_global_ptr->get(find_match).Array()->size() - 1) / 3 - 1;
          for (size_t i = 0; i < parentheses_match_size; i++) {
            Value* p = this_obj + (++param_len);
            *p = Value(array_global_ptr->get(find_match)
                           .Array()
                           ->get(3 * i + 4)
                           .String());
          }

          Value* offset = this_obj + (++param_len);
          offset->SetNumber(static_cast<int64_t>(
              array_global_ptr->get(find_match).Array()->get(2).Number()));
          Value* string = this_obj + (++param_len);
          *string =
              Value(array_global_ptr->get(find_match).Array()->get(0).String());
          Value call_function_ret;
          static_cast<VMContext*>(context)->CallFunction(
              call_function, param_len + 1, &call_function_ret);
          find_match++;
          str_to_replace = call_function_ret.StdString();
          str_to_replace_unicode_len = str_to_replace.length();
          GetReplaceResult(str_to_replace, result, has_unicode,
                           str_to_replace_has_unicode,
                           str_to_replace_unicode_len, match_start, match_end);
        } else {
          str_to_replace = GetReplaceStr(param2_str, *array_global_ptr,
                                         find_match, str, bc, global_mode);
          str_to_replace_unicode_len = str_to_replace.length();
          find_match++;
          GetReplaceResult(str_to_replace, result, has_unicode,
                           str_to_replace_has_unicode,
                           str_to_replace_unicode_len, match_start, match_end);
        }
      }
      input_len = result.length();
      if (global_mode) {
        if (str_to_replace_has_unicode) {
          start_search_index = match_end + (str_to_replace_unicode_len -
                                            (match_end - match_start));
        } else {
          start_search_index =
              match_end + (str_to_replace.length() - (match_end - match_start));
        }
      } else {
        start_search_index = input_len + 1;
      }
    }
    free(bc);
  }
  return Value(std::move(result));
}

static Value Slice(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2 || params_count == 3);

  const std::string& str = context->GetParam(params_count - 1)->StdString();
  if (params_count == 1) {
    return Value(str);
  }

  int64_t startIndex = static_cast<int64_t>(context->GetParam(0)->Number());
  if (startIndex < 0) {
    size_t size_c =
        base::CIndexToUTF8Index(str.c_str(), str.length(), str.size());
    startIndex = size_c + startIndex;
    startIndex = startIndex < 0 ? 0 : startIndex;
  }
  size_t start_index = base::UTF8IndexToCIndex(str.c_str(), str.length(),
                                               static_cast<size_t>(startIndex));
  size_t strIndex = start_index >= str.size() ? str.size() : start_index;

  if (params_count == 2) {
    return Value(str.substr(strIndex));
  } else {
    int64_t endIndex = static_cast<int64_t>(context->GetParam(1)->Number());
    if (endIndex < 0) {
      size_t size_c =
          base::CIndexToUTF8Index(str.c_str(), str.length(), str.size());
      endIndex = size_c + endIndex;
      endIndex = endIndex < 0 ? 0 : endIndex;
    }
    size_t end_index = base::UTF8IndexToCIndex(str.c_str(), str.length(),
                                               static_cast<size_t>(endIndex));
    if (start_index >= end_index) {
      return Value(base::String());
    }
    return Value(str.substr(start_index, end_index - start_index));
  }
}

static Value SubString(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(context->GetParam(params_count - 1)->IsString());
  const std::string& str = context->GetParam(params_count - 1)->StdString();
  DCHECK(params_count == 2 || params_count == 3);
  DCHECK(context->GetParam(0)->IsNumber());

  int32_t start = static_cast<int32_t>(context->GetParam(0)->Number());
  if (params_count == 2) {
    start = start < 0 ? 0 : start;
    start = static_cast<size_t>(start) > str.size()
                ? static_cast<int32_t>(str.size())
                : start;
    size_t start_index =
        base::UTF8IndexToCIndex(str.c_str(), str.length(), start);
    return Value(str.substr(start_index));
  } else {
    DCHECK(context->GetParam(1)->IsNumber());
    int32_t end = static_cast<int32_t>(context->GetParam(1)->Number());
    if (start > end) {
      std::swap(start, end);
    }
    start = start < 0 ? 0 : start;
    start = static_cast<size_t>(start) > str.size()
                ? static_cast<int32_t>(str.size())
                : start;
    size_t start_index =
        base::UTF8IndexToCIndex(str.c_str(), str.length(), start);

    end = end < 0 ? 0 : end;
    end = static_cast<size_t>(end) > str.size()
              ? static_cast<int32_t>(str.size())
              : end;
    size_t end_index = base::UTF8IndexToCIndex(str.c_str(), str.length(), end);
    return Value(str.substr(start_index, end_index - start_index));
  }
}

static Value IndexOf(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count > 1);
  Value* this_obj = context->GetParam(0);
  Value* arg = context->GetParam(1);
  long index = params_count == 2 ? 0 : context->GetParam(2)->Number();

  if (this_obj->IsString() && arg->IsString()) {
    const auto& this_str = this_obj->StdString();
    std::size_t result = this_str.find(arg->StdString(), index);
    if (result != std::string::npos) {
      return Value(static_cast<uint32_t>(base::CIndexToUTF8Index(
          this_str.c_str(), this_str.length(), result)));
    }
  }
  return Value(-1);
}

static Value Length(VMContext* context) {
  DCHECK(context->GetParam(0)->IsString());
  const auto& str = context->GetParam(0)->StdString();
  return Value(
      static_cast<uint32_t>(base::SizeOfUtf8(str.c_str(), str.length())));
}

// substr(start[, length])
static Value SubStr(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 3 || params_count == 2);
  DCHECK(context->GetParam(0)->IsString());
  DCHECK(context->GetParam(1)->IsNumber());
  auto str = context->GetParam(0)->String();

  int64_t start = static_cast<int64_t>(context->GetParam(1)->Number());
  size_t utf8_start_index = static_cast<size_t>(
      start < 0 ? (static_cast<size_t>(abs(start)) > str.length_utf8()
                       ? 0
                       : static_cast<int64_t>(str.length_utf8()) + start)
                : start);
  size_t start_index =
      base::UTF8IndexToCIndex(str.c_str(), str.length(), utf8_start_index);
  if (params_count == 3) {
    DCHECK(context->GetParam(2)->IsNumber());
    int64_t length = static_cast<int64_t>(context->GetParam(2)->Number());
    if (length <= 0) {
      return Value(base::String());
    }
    size_t end_index =
        base::UTF8IndexToCIndex(str.c_str(), str.length(),
                                utf8_start_index + static_cast<size_t>(length));
    return Value(str.str().substr(start_index, end_index - start_index));
  } else {
    return Value(str.str().substr(start_index));
  }
}

static Value Split(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2 || params_count == 3);

  auto str_arg = context->GetParam(params_count - 1)->String();
  const std::string& str = str_arg.str();
  const std::string& pattern = context->GetParam(0)->StdString();

  size_t pattern_length = pattern.length(), str_length = str.length();
  Value res = Value(CArray::Create()), temp;
  auto array_res = res.Array();
  size_t max_size = 0, size = 0;
  bool max_flag = false;
  if (params_count == 3) {
    max_size = static_cast<size_t>(context->GetParam(1)->Number());
    max_flag = true;
  } else if (params_count == 1) {
    array_res->emplace_back(str_arg);
    return res;
  }
  if (str_length == 0) {
    if (pattern_length != 0) {
      if (max_size || !max_flag) {
        array_res->emplace_back(base::String());
      }
    }
  } else if (pattern_length == 0) {
    // Calculate UTF8 length first and reserve for array.
    array_res->reserve(base::SizeOfUtf8(str.c_str(), str.length()));
    for (size_t i = 0; i < str_length;) {
      if (max_flag && size == max_size) break;
      size_t cur_length = base::InlineUTF8SequenceLength(str[i]);
      array_res->emplace_back(base::String(&str[i], cur_length));
      i += cur_length;
      size++;
    }
    std::cout << std::endl;
  } else {
    std::string strs = str + pattern;
    size_t pos = strs.find(pattern);
    while (pos != strs.npos) {
      if (max_flag && size == max_size) break;
      array_res->emplace_back(strs.substr(0, pos));
      size++;
      strs = strs.substr(pos + pattern_length, strs.size());
      pos = strs.find(pattern);
    }
  }
  return res;
}

void RegisterStringAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "indexOf", &IndexOf);
  RegisterTableFunction(ctx, table, "length", &Length);
  RegisterTableFunction(ctx, table, "substr", &SubStr);
  RegisterFunctionTable(ctx, "String", std::move(table));
}

void RegisterStringPrototypeAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "split", &Split);
  RegisterTableFunction(ctx, table, "trim", &Trim);
  RegisterTableFunction(ctx, table, "charAt", &CharAt);
  RegisterTableFunction(ctx, table, "search", &Search);
  RegisterTableFunction(ctx, table, "match", &Match);
  RegisterTableFunction(ctx, table, "replace", &Replace);
  RegisterTableFunction(ctx, table, "slice", &Slice);
  RegisterTableFunction(ctx, table, "substring", &SubString);
  reinterpret_cast<VMContext*>(ctx)->SetStringPrototype(
      Value(std::move(table)));
}

}  // namespace lepus
}  // namespace lynx
