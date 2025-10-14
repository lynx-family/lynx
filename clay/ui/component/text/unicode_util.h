// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_UNICODE_UTIL_H_
#define CLAY_UI_COMPONENT_TEXT_UNICODE_UTIL_H_

#include <string>

#include "base/include/string/string_utils.h"

class UnicodeUtil {
 public:
  // support icon-font
  // e.g. "&#xE134;今天天气good#xE134;" -> "\uE134今天天气good\uxE134"
  static std::u16string Utf8ToUtf16(const std::string& str) {
    const std::string identify = "&#";
    int start = 0;
    int idx = str.find(identify, start);
    if (idx != -1) {
      int len = str.length();
      std::string substr;
      std::u16string str_16;
      const char* data = str.data();
      while (idx != -1) {
        substr = str.substr(start, idx - start);
        str_16 += lynx::base::U8StringToU16(substr);
        char* src = const_cast<char*>(data) + idx;
        char* end = 0;
        if (idx + 2 < len && str.at(idx + 2) == 'x') {
          unsigned int code_point = strtoul(src + 3, &end, 16);
          str_16 += (char16_t)code_point;
        } else {
          unsigned int code_point = strtoul(src + 2, &end, 10);
          str_16 += (char16_t)code_point;
        }

        // reset
        start = end - data;
        if (data[start] == ';') {
          // skip ; e.g. &#xE813; -> \uE813
          start++;
        }
        idx = str.find(identify, start);
      }
      if (start != len) {
        substr = str.substr(start);
        str_16 += lynx::base::U8StringToU16(substr);
      }
      return str_16;
    }
    return lynx::base::U8StringToU16(str);
  }
};

#endif  // CLAY_UI_COMPONENT_TEXT_UNICODE_UTIL_H_
