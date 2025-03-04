// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_STRING_STRING_CONVERSION_WIN_H_
#define BASE_INCLUDE_STRING_STRING_CONVERSION_WIN_H_

#include <string>
#include <string_view>

namespace lynx {
namespace base {

// Converts a string from UTF-16 to UTF-8. Returns an empty string if the
// input is not valid UTF-16.
std::string Utf8FromUtf16(const std::wstring& utf16_string);
std::string Utf8FromUtf16(const std::wstring_view& utf8_string);
std::string Utf8FromUtf16(const wchar_t* utf16_string, int32_t length);

// Converts a string from UTF-8 to UTF-16. Returns an empty string if the
// input is not valid UTF-8.
std::wstring Utf16FromUtf8(const std::string& utf8_string);
std::wstring Utf16FromUtf8(const std::string_view& utf8_string);
std::wstring Utf16FromUtf8(const char* utf8_string, int32_t length);

// Converts a string from UTF-8 to ANSI or OEM .
// This function was originally designed  because older V8 code uses Fopen and
// only allows you to pass const char* argument. Like
// v8::V8::InitializeExternalStartupData;
// See :
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-170
std::string Utf8ToANSIOrOEM(const std::string& utf8_string);

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_STRING_STRING_CONVERSION_WIN_H_
