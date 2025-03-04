// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/string/string_conversion_win.h"

#include <Windows.h>

namespace lynx {
namespace base {

std::string Utf8FromUtf16(const std::wstring& utf16_string) {
  return Utf8FromUtf16(utf16_string.data(), utf16_string.length());
}

std::string Utf8FromUtf16(const std::wstring_view& utf16_string) {
  return Utf8FromUtf16(utf16_string.data(), utf16_string.length());
}

std::string Utf8FromUtf16(const wchar_t* utf16_string, int32_t length) {
  if (!utf16_string || length <= 0) {
    return {};
  }
  int target_length =
      ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16_string, length,
                            nullptr, 0, nullptr, nullptr);
  if (target_length == 0) {
    return {};
  }
  std::string utf8_string;
  utf8_string.resize(target_length);
  int converted_length = ::WideCharToMultiByte(
      CP_UTF8, WC_ERR_INVALID_CHARS, utf16_string, length,
      const_cast<LPSTR>(utf8_string.c_str()), target_length, nullptr, nullptr);
  if (converted_length == 0) {
    return {};
  }
  return utf8_string;
}

std::wstring Utf16FromUtf8(const std::string& utf8_string) {
  return Utf16FromUtf8(utf8_string.data(),
                       static_cast<int>(utf8_string.length()));
}

std::wstring Utf16FromUtf8(const std::string_view& utf8_string) {
  return Utf16FromUtf8(utf8_string.data(),
                       static_cast<int>(utf8_string.length()));
}

std::wstring Utf16FromUtf8(const char* utf8_string, int32_t length) {
  if (!utf8_string || length <= 0) {
    return {};
  }

  int target_length = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            utf8_string, length, nullptr, 0);
  if (target_length == 0) {
    return {};
  }
  std::wstring utf16_string;
  utf16_string.resize(target_length);
  int converted_length = ::MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, utf8_string, length,
      const_cast<LPWSTR>(utf16_string.c_str()), target_length);
  if (converted_length == 0) {
    return {};
  }
  return utf16_string;
}

std::string Utf8ToANSIOrOEM(const std::string& utf8_string) {
  std::wstring utf16_str = Utf16FromUtf8(utf8_string);
  if (utf16_str.empty()) {
    return {};
  }

  UINT type = CP_ACP;
  if (!::AreFileApisANSI()) {
    type = CP_OEMCP;
  }

  int target_length = ::WideCharToMultiByte(
      type, WC_NO_BEST_FIT_CHARS, utf16_str.c_str(),
      static_cast<int>(utf16_str.length()), nullptr, 0, nullptr, nullptr);
  if (target_length == 0) {
    return {};
  }

  std::string ansi_string;
  ansi_string.resize(target_length);
  int converted_length = ::WideCharToMultiByte(
      type, WC_NO_BEST_FIT_CHARS, utf16_str.data(),
      static_cast<int>(utf16_str.length()),
      const_cast<LPSTR>(ansi_string.c_str()), target_length, nullptr, nullptr);
  if (converted_length == 0) {
    return {};
  }

  return ansi_string;
}

}  // namespace base
}  // namespace lynx
